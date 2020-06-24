package auth

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"html/template"
	"net/http"
	"net/url"
	"os"
	"os/user"
	"path/filepath"
	"strings"
	"time"

	"github.com/dgrijalva/jwt-go"
	log "github.com/sirupsen/logrus"
	"github.com/skratchdot/open-golang/open"
	"google.golang.org/grpc/metadata"
	"gopkg.in/segmentio/analytics-go.v3"
	"pixielabs.ai/pixielabs/src/cloud/cloudapipb"
	"pixielabs.ai/pixielabs/src/utils"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/components"
	utils2 "pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/utils"

	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/pxanalytics"
	"pixielabs.ai/pixielabs/src/utils/pixie_cli/pkg/pxconfig"
)

const pixieAuthPath = ".pixie"
const pixieAuthFile = "auth.json"

var errUserChallengeTimeout = errors.New("timeout waiting for user")
var errBrowserFailed = errors.New("browser failed to open")
var errTokenUnauthorized = errors.New("failed to obtain token")
var localServerRedirectURL = "http://localhost:8085/auth_complete"
var localServerPort = int32(8085)

const authCompletePage = `
<!DOCTYPE HTML>
<html lang="en-US">
  <head>
    <meta charset="UTF-8">
    <script type="text/javascript">
      window.location.href = "{{ .CloudAddr }}"
    </script>
    <title>Authentication Successful - Pixie</title>
  </head>
  <body>
    <p><font face=roboto>
      You may close this window.
    </font></p>
  </body>
</html>
`

// Template of the page to render when auth succeeds.
var authCompleteTmpl = template.Must(template.New("authCompletePage").Parse(authCompletePage))

// EnsureDefaultAuthFilePath returns and creates the file path is missing.
func EnsureDefaultAuthFilePath() (string, error) {
	u, err := user.Current()
	if err != nil {
		return "", err
	}

	pixieDirPath := filepath.Join(u.HomeDir, pixieAuthPath)
	if _, err := os.Stat(pixieDirPath); os.IsNotExist(err) {
		os.Mkdir(pixieDirPath, 0744)
	}

	pixieAuthFilePath := filepath.Join(pixieDirPath, pixieAuthFile)
	return pixieAuthFilePath, nil
}

// SaveRefreshToken saves the refresh token in default spot.
func SaveRefreshToken(token *RefreshToken) error {
	pixieAuthFilePath, err := EnsureDefaultAuthFilePath()
	if err != nil {
		return err
	}

	f, err := os.OpenFile(pixieAuthFilePath, os.O_RDWR|os.O_CREATE, 0600)
	if err != nil {
		return err
	}
	defer f.Close()

	return json.NewEncoder(f).Encode(token)
}

// LoadDefaultCredentials loads the default credentials for the user.
func LoadDefaultCredentials() (*RefreshToken, error) {
	pixieAuthFilePath, err := EnsureDefaultAuthFilePath()
	if err != nil {
		return nil, err
	}
	f, err := os.Open(pixieAuthFilePath)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	token := &RefreshToken{}
	if err := json.NewDecoder(f).Decode(token); err != nil {
		return nil, err
	}
	_ = pxanalytics.Client().Enqueue(&analytics.Track{
		UserId: pxconfig.Cfg().UniqueClientID,
		Event:  "Load Stored Creds",
	})

	if token, _ := jwt.Parse(token.Token, nil); token != nil {
		sc, ok := token.Claims.(jwt.MapClaims)
		if ok {
			userID, _ := sc["UserID"].(string)
			// Associate UserID with AnalyticsID.
			_ = pxanalytics.Client().Enqueue(&analytics.Alias{
				UserId:     pxconfig.Cfg().UniqueClientID,
				PreviousId: userID,
			})
			_ = pxanalytics.Client().Enqueue(&analytics.Identify{
				UserId: userID,
			})
		}
	}

	if token.SupportAccount {
		components.RenderBureaucratDragon(token.OrgName)
	}

	// TODO(zasgar): Exchange refresh token for new token type.
	return token, nil
}

// PixieCloudLogin performs login on the pixie cloud.
type PixieCloudLogin struct {
	ManualMode bool
	CloudAddr  string
	// OrgName: Selection is only valid for "pixie.support", will be removed when RBAC is supported.
	OrgName string
}

// Run either launches the browser or prints out the URL for auth.
func (p *PixieCloudLogin) Run() (*RefreshToken, error) {
	// There are two ways to do the auth. The first one is where we automatically open up the browser
	// and wait for the challenge to complete and call a HTTP server that we started.
	// The second one is to perform a manual auth.
	// Unless manual mode is specified we will try perform the browser based auth and fallback to manual auth.
	if !p.ManualMode {
		refreshToken, err := p.tryBrowserAuth()
		// Handle errors.
		switch err {
		case nil:
			return refreshToken, nil
		case errUserChallengeTimeout:
			log.Fatal("Timeout waiting for response from browser. Perhaps try --manual mode.")
		case errBrowserFailed:
			fallthrough
		default:
			log.Infof("err: %v", err)
			log.Info("Failed to perform browser based auth. Will try manual auth")
		}
	}
	_ = pxanalytics.Client().Enqueue(&analytics.Track{
		UserId: pxconfig.Cfg().UniqueClientID,
		Event:  "Manual Auth",
	})
	// Try to request using manual mode
	accessToken, err := p.getAuthStringManually()
	if err != nil {
		return nil, err
	}
	log.Info("Fetching refresh token")

	return p.getRefreshToken(accessToken)
}

func addCORSHeaders(res http.ResponseWriter) {
	headers := res.Header()
	headers.Add("Access-Control-Allow-Origin", "*")
	headers.Add("Vary", "Origin")
	headers.Add("Vary", "Access-Control-Request-Method")
	headers.Add("Vary", "Access-Control-Request-Headers")
	headers.Add("Access-Control-Allow-Headers", "Content-Type, Origin, Accept, token")
	headers.Add("Access-Control-Allow-Methods", "GET, POST,OPTIONS")
}

func (p *PixieCloudLogin) tryBrowserAuth() (*RefreshToken, error) {
	// Browser auth starts up a server on localhost to do the user challenge
	// and get the authentication token.
	_ = pxanalytics.Client().Enqueue(&analytics.Track{
		UserId: pxconfig.Cfg().UniqueClientID,
		Event:  "Browser Auth",
	})
	authURL := p.getAuthURL()
	q := authURL.Query()
	q.Set("redirect_uri", localServerRedirectURL)
	authURL.RawQuery = q.Encode()

	fmt.Printf("Opening authentication URL: %s\n", authURL.String())

	type result struct {
		Token *RefreshToken
		err   error
	}

	// The token/ error is returned on this channel. A closed channel also implies error.
	results := make(chan result, 1)

	mux := http.DefaultServeMux
	// Start up HTTP server to intercept the browser data.
	mux.HandleFunc("/auth_complete", func(w http.ResponseWriter, r *http.Request) {
		addCORSHeaders(w)
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusOK)
			return
		} else if r.Method != http.MethodPost {
			results <- result{nil, errors.New("wrong method on HTTP request, assuming auth failed")}
			close(results)
			return
		}

		if err := r.ParseForm(); err != nil {
			close(results)
			return
		}

		accessToken := r.Form.Get("access_token")
		if accessToken == "" {
			results <- result{nil, errors.New("missing code, assuming auth failed")}
			close(results)
			return
		}

		refreshToken, err := p.getRefreshToken(accessToken)

		// Fill out the template with the correct data.
		templateParams := struct {
			CloudAddr string
		}{getAuthCompleteURL(fmt.Sprintf("work.%s", p.CloudAddr), err)}

		// Write out the page to the handler.
		authCompleteTmpl.Execute(w, templateParams)

		if err != nil {
			results <- result{nil, err}
			close(results)
			return
		}

		// Sucessful auth.
		results <- result{refreshToken, nil}
		close(results)
	})

	h := http.Server{
		Addr:    fmt.Sprintf(":%d", localServerPort),
		Handler: mux,
	}

	// Start up the server in the background. Wait for either a timeout
	// or completion of the challenge auth.
	go func() {
		if err := h.ListenAndServe(); err != nil {
			if err == http.ErrServerClosed {
				return
			}
			log.WithError(err).Fatal("failed to listen")
		}
	}()

	go func() {
		log.Info("Starting browser")
		err := open.Run(authURL.String())
		if err != nil {
			_ = pxanalytics.Client().Enqueue(&analytics.Track{
				UserId: pxconfig.Cfg().UniqueClientID,
				Event:  "Browser Open Failed",
			})
			results <- result{nil, errBrowserFailed}
			close(results)
		}
	}()

	ctx, cancel := context.WithTimeout(context.Background(), time.Minute*2)
	defer cancel()
	defer h.Shutdown(ctx)
	for {
		select {
		case <-ctx.Done():
			return nil, errUserChallengeTimeout
		case res, ok := <-results:
			if !ok {
				_ = pxanalytics.Client().Enqueue(&analytics.Track{
					UserId: pxconfig.Cfg().UniqueClientID,
					Event:  "Auth Failure",
				})
				return nil, errUserChallengeTimeout
			}
			_ = pxanalytics.Client().Enqueue(&analytics.Track{
				UserId: pxconfig.Cfg().UniqueClientID,
				Event:  "Auth Success",
			})
			// TODO(zasgar): This is a hack, figure out why this function takes so long to exit.
			log.Info("Fetching refresh token ...")
			return res.Token, res.err
		}
	}
}

func (p *PixieCloudLogin) getAuthStringManually() (string, error) {
	authURL := p.getAuthURL()
	fmt.Printf("\nPlease Visit: \n \t %s\n\n", authURL.String())
	f := bufio.NewWriter(os.Stdout)
	f.WriteString("Copy and paste token here: ")
	f.Flush()

	r := bufio.NewReader(os.Stdin)
	return r.ReadString('\n')
}

func (p *PixieCloudLogin) getRefreshToken(accessToken string) (*RefreshToken, error) {
	params := struct {
		AccessToken string `json:"accessToken"`
		OrgName     string `json:"orgName,omitempty"`
	}{
		AccessToken: strings.Trim(accessToken, "\n"),
		OrgName:     p.OrgName,
	}
	b, err := json.Marshal(params)
	if err != nil {
		return nil, err
	}
	authURL := p.getAuthAPIURL()
	req, err := http.NewRequest("POST", authURL, bytes.NewBuffer(b))
	req.Header.Set("content-type", "application/json")
	if err != nil {
		return nil, err
	}

	client := http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusUnauthorized || resp.StatusCode == http.StatusBadRequest {
		return nil, errTokenUnauthorized
	}
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("Request for token failed with status %d", resp.StatusCode)
	}
	refreshToken := &RefreshToken{}
	if err := json.NewDecoder(resp.Body).Decode(refreshToken); err != nil {
		return nil, err
	}

	refreshToken.SupportAccount = p.OrgName != ""
	refreshToken.OrgName = p.OrgName

	if refreshToken.OrgName == "" {
		// Get the org name from the cloud.
		var orgID string
		if token, _ := jwt.Parse(refreshToken.Token, nil); token != nil {
			sc, ok := token.Claims.(jwt.MapClaims)
			if ok {
				orgID, _ = sc["OrgID"].(string)
			}
		}

		uuidProto := utils.ProtoFromUUIDStrOrNil(orgID)
		conn, err := utils2.GetCloudClientConnection(p.CloudAddr)
		if err != nil {
			return nil, err
		}
		client := cloudapipb.NewProfileServiceClient(conn)
		ctx := context.Background()
		ctx = metadata.AppendToOutgoingContext(context.Background(), "authorization",
			fmt.Sprintf("bearer %s", refreshToken.Token))
		ctx, cancel := context.WithTimeout(ctx, 2*time.Second)
		defer cancel()
		resp, err := client.GetOrgInfo(ctx, uuidProto)
		if err != nil {
			return nil, err
		}
		refreshToken.OrgName = resp.OrgName
	}
	return refreshToken, nil
}

// RefreshToken is the format for the refresh token.
type RefreshToken struct {
	Token          string `json:"token"`
	ExpiresAt      int64  `json:"expiresAt"`
	SupportAccount bool   `json:"supportAccount,omitempty"`
	OrgName        string `json:"orgName,omitempty"`
}

func (p *PixieCloudLogin) getAuthURL() *url.URL {
	authURL, err := url.Parse(fmt.Sprintf("https://work.%s", p.CloudAddr))
	if err != nil {
		log.WithError(err).Fatal("Failed to parse cloud addr.")
	}
	authURL.Path = "/login"
	params := url.Values{}
	params.Add("local_mode", "true")
	if len(p.OrgName) > 0 {
		params.Add("org_name", p.OrgName)
	}
	authURL.RawQuery = params.Encode()
	return authURL
}

func (p *PixieCloudLogin) getAuthAPIURL() string {
	authURL, err := url.Parse(fmt.Sprintf("https://%s/api/auth/login", p.CloudAddr))
	if err != nil {
		log.WithError(err).Fatal("Failed to parse cloud addr.")
	}
	return authURL.String()
}

func getAuthCompleteURL(cloudAddr string, err error) string {
	authURL := &url.URL{
		Scheme: "https",
		Host:   cloudAddr,
		Path:   "/auth-complete",
	}
	if err == nil {
		return authURL.String()
	}
	params := url.Values{}
	if err == errTokenUnauthorized {
		params.Add("err", "token")
	} else {
		params.Add("err", "true")
	}
	authURL.RawQuery = params.Encode()
	return authURL.String()
}