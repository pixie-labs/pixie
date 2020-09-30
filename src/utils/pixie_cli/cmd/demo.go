package cmd

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/fatih/color"
	log "github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	analytics "gopkg.in/segmentio/analytics-go.v3"
	v1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	k8s_errors "k8s.io/apimachinery/pkg/api/errors"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/components"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/pxanalytics"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/pxconfig"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/utils"
	"pixielabs.ai/pixielabs/src/utils/shared/k8s"
)

const manifestFile = "manifest.json"

var errNamespaceAlreadyExists = errors.New("namespace already exists")

// DemoCmd is the demo sub-command of the CLI to deploy and delete demo apps.
var DemoCmd = &cobra.Command{
	Use:   "demo",
	Short: "Manage demo apps",
	Run: func(cmd *cobra.Command, args []string) {
		log.Info("Nothing here... Please execute one of the subcommands")
		cmd.Help()
		return
	},
}

var listDemoCmd = &cobra.Command{
	Use:   "list",
	Short: "List available demo apps",
	Run:   listCmd,
	PreRun: func(cmd *cobra.Command, args []string) {
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo List Apps",
		})
	},
	PostRun: func(cmd *cobra.Command, args []string) {
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo List Apps Complete",
		})
	},
}

var deleteDemoCmd = &cobra.Command{
	Use:   "delete",
	Short: "Delete demo app",
	Args:  cobra.ExactArgs(1),
	Run:   deleteCmd,
	PreRun: func(cmd *cobra.Command, args []string) {
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Delete App",
			Properties: analytics.NewProperties().
				Set("app", args[0]),
		})
	},
	PostRun: func(cmd *cobra.Command, args []string) {
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Delete App Complete",
			Properties: analytics.NewProperties().
				Set("app", args[0]),
		})
	},
}

var deployDemoCmd = &cobra.Command{
	Use:   "deploy",
	Short: "Deploy demo app",
	Args:  cobra.ExactArgs(1),
	Run:   deployCmd,
	PreRun: func(cmd *cobra.Command, args []string) {
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Deploy App",
			Properties: analytics.NewProperties().
				Set("app", args[0]),
		})
	},
	PostRun: func(cmd *cobra.Command, args []string) {
		defer pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Deploy App Complete",
			Properties: analytics.NewProperties().
				Set("app", args[0]),
		})
	},
}

func init() {
	DemoCmd.PersistentFlags().String("artifacts", "https://storage.googleapis.com/pixie-prod-artifacts/prod-demo-apps", "The path to the demo apps")
	viper.BindPFlag("artifacts", DemoCmd.PersistentFlags().Lookup("artifacts"))

	DemoCmd.AddCommand(listDemoCmd)
	DemoCmd.AddCommand(deployDemoCmd)
	DemoCmd.AddCommand(deleteDemoCmd)
}

func listCmd(cmd *cobra.Command, args []string) {
	var err error
	defer func() {
		if err == nil {
			return
		}
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo List Apps Error",
			Properties: analytics.NewProperties().
				Set("error", err.Error()),
		})
	}()

	manifest, err := downloadManifest(viper.GetString("artifacts"))
	if err != nil {
		log.WithError(err).Fatal("Could not download manifest file")
	}

	w := components.CreateStreamWriter("table", os.Stdout)
	defer w.Finish()
	w.SetHeader("demo_list", []string{"Name"})
	for app, appSpec := range manifest {
		// When a demo app is deprecated, its contents will be set to null in manifest.json.
		if appSpec != nil {
			w.Write([]interface{}{app})
		}
	}
}

func deleteCmd(cmd *cobra.Command, args []string) {
	appName := args[0]

	var err error
	defer func() {
		if err == nil {
			return
		}
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Delete App Error",
			Properties: analytics.NewProperties().
				Set("app", appName).
				Set("error", err.Error()),
		})
	}()

	manifest, err := downloadManifest(viper.GetString("artifacts"))
	if err != nil {
		log.WithError(err).Fatal("Could not download manifest file")
	}
	if _, ok := manifest[appName]; !ok {
		log.Fatalf("%s is not a supported demo app", appName)
	}

	currentCluster := getCurrentCluster()
	fmt.Printf("Deleting demo app %s from the following cluster: %s\n", appName, currentCluster)
	clusterOk := components.YNPrompt("Is the cluster correct?", true)
	if !clusterOk {
		log.Fatal("Cluster is not correct. Aborting.")
	}

	if !namespaceExists(appName) {
		log.Fatalf("Namespace %s does not exist on cluster %s", appName, currentCluster)
	}

	if err = deleteDemoApp(appName); err != nil {
		log.WithError(err).Fatalf("Error deleting demo app %s from cluster %s", appName, currentCluster)
	} else {
		log.Infof("Successfully deleted demo app %s from cluster %s", appName, currentCluster)
	}
}

func deployCmd(cmd *cobra.Command, args []string) {
	appName := args[0]

	var err error
	defer func() {
		if err == nil {
			return
		}
		pxanalytics.Client().Enqueue(&analytics.Track{
			UserId: pxconfig.Cfg().UniqueClientID,
			Event:  "Demo Deploy App Error",
			Properties: analytics.NewProperties().
				Set("app", appName).
				Set("error", err.Error()),
		})
	}()

	manifest, err := downloadManifest(viper.GetString("artifacts"))
	if err != nil {
		log.WithError(err).Fatal("Could not download manifest file")
	}

	appSpec, ok := manifest[appName]
	// When a demo app is deprecated, its contents will be set to null in manifest.json.
	if !ok || appSpec == nil {
		log.Fatalf("%s is not a supported demo app", appName)
	}
	instructions := strings.Join(appSpec.Instructions, "\n")

	yamls, err := downloadDemoAppYAMLs(appName, viper.GetString("artifacts"))
	if err != nil {
		log.WithError(err).Fatal("Could not download demo yaml apps for app '%s'", appName)
	}

	currentCluster := getCurrentCluster()
	fmt.Printf("Deploying demo app %s to the following cluster: %s\n", appName, currentCluster)
	clusterOk := components.YNPrompt("Is the cluster correct?", true)
	if !clusterOk {
		log.Error("Cluster is not correct. Aborting.")
		return
	}

	err = setupDemoApp(appName, yamls)
	if err != nil {
		if errors.Is(err, errNamespaceAlreadyExists) {
			log.Fatalf("Failed to deploy demo application: namespace already exists.")
		}
		log.WithError(err).Errorf("Error deploying demo application, deleting namespace %s", appName)
		// Note: If you can specify the namespace for the demo app in the future, we shouldn't delete the namespace.
		if err = deleteDemoApp(appName); err != nil {
			log.WithError(err).Errorf("Error deleting namespace %s", appName)
		}
		log.Fatalf("Failed to deploy demo application.")
	}

	fmt.Fprintf(os.Stderr, "Successfully deployed demo app %s to cluster %s.\n", args[0], currentCluster)

	p := func(s string, a ...interface{}) {
		fmt.Fprintf(os.Stderr, s, a...)
	}
	b := color.New(color.Bold)
	p(color.CyanString("==> ") + b.Sprint("Next Steps:\n\n"))
	p(instructions)
}

type manifestAppSpec struct {
	Instructions []string `json:"instructions"`
}

type manifest = map[string]*manifestAppSpec

func downloadGCSFileFromHTTP(dirURL, filename string) ([]byte, error) {
	// Get the data
	resp, err := http.Get(fmt.Sprintf("%s/%s", dirURL, filename))
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	return ioutil.ReadAll(resp.Body)
}

func downloadManifest(artifacts string) (manifest, error) {
	jsonBytes, err := downloadGCSFileFromHTTP(artifacts, manifestFile)
	if err != nil {
		return nil, err
	}

	jsonManifest := make(manifest)
	err = json.Unmarshal(jsonBytes, &jsonManifest)
	if err != nil {
		return nil, err
	}
	return jsonManifest, nil
}

func deleteDemoApp(appName string) error {
	deleteDemo := []utils.Task{
		newTaskWrapper(fmt.Sprintf("Deleting demo app %s", appName), func() error {
			kubeConfig := k8s.GetConfig()
			clientset := k8s.GetClientset(kubeConfig)

			err := clientset.CoreV1().Namespaces().Delete(context.Background(), appName, metav1.DeleteOptions{})
			if err != nil {
				return err
			}
			t := time.NewTimer(180 * time.Second)
			defer t.Stop()

			s := time.NewTicker(5 * time.Second)
			defer s.Stop()

			for {
				select {
				case <-t.C:
					return errors.New("timeout waiting for namespace deletion")
				default:
					_, err := clientset.CoreV1().Namespaces().Get(context.Background(), appName, metav1.GetOptions{})
					if k8s_errors.IsNotFound(err) {
						return nil
					}
					if err != nil {
						return err
					}
					<-s.C
				}

			}
		}),
	}
	tr := utils.NewSerialTaskRunner(deleteDemo)
	return tr.RunAndMonitor()
}

func downloadDemoAppYAMLs(appName, artifacts string) (map[string][]byte, error) {
	targzBytes, err := downloadGCSFileFromHTTP(artifacts, fmt.Sprintf("%s.tar.gz", appName))
	if err != nil {
		return nil, err
	}
	gzipReader, err := gzip.NewReader(bytes.NewReader(targzBytes))
	if err != nil {
		return nil, err
	}
	defer gzipReader.Close()

	tarReader := tar.NewReader(gzipReader)
	outputYAMLs := map[string][]byte{}

	for {
		hdr, err := tarReader.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			return nil, err
		}

		if !strings.HasSuffix(hdr.Name, ".yaml") {
			continue
		}

		contents, err := ioutil.ReadAll(tarReader)
		if err != nil {
			return nil, err
		}
		outputYAMLs[hdr.Name] = contents
	}
	return outputYAMLs, nil
}

func namespaceExists(namespace string) bool {
	kubeConfig := k8s.GetConfig()
	clientset := k8s.GetClientset(kubeConfig)
	_, err := clientset.CoreV1().Namespaces().Get(context.Background(), namespace, metav1.GetOptions{})
	return err == nil
}

func createNamespace(namespace string) error {
	kubeConfig := k8s.GetConfig()
	clientset := k8s.GetClientset(kubeConfig)
	_, err := clientset.CoreV1().Namespaces().Create(context.Background(), &v1.Namespace{ObjectMeta: metav1.ObjectMeta{Name: namespace}}, metav1.CreateOptions{})
	return err
}

func setupDemoApp(appName string, yamls map[string][]byte) error {
	kubeConfig := k8s.GetConfig()
	clientset := k8s.GetClientset(kubeConfig)
	if namespaceExists(appName) {
		fmt.Printf("%s: namespace %s already exists. If created with px, run %s to remove\n",
			color.RedString("Error"), color.RedString(appName), color.GreenString((fmt.Sprintf("px demo delete %s", appName))))
		return errNamespaceAlreadyExists
	}

	tasks := []utils.Task{
		newTaskWrapper(fmt.Sprintf("Creating namespace %s", appName), func() error {
			return createNamespace(appName)
		}),
		newTaskWrapper(fmt.Sprintf("Deploying %s YAMLs", appName), func() error {
			for _, yamlBytes := range yamls {
				yamlBytes := yamlBytes
				err := k8s.ApplyYAML(clientset, kubeConfig, appName, bytes.NewReader(yamlBytes), false)
				if err != nil {
					return err
				}
			}
			return nil
		}),
	}

	tr := utils.NewSerialTaskRunner(tasks)
	return tr.RunAndMonitor()
}