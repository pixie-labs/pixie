package services

import (
	"net/http"
	"os"
	"time"

	log "github.com/sirupsen/logrus"
	prefixed "github.com/x-cray/logrus-prefixed-formatter"
	"github.com/zenazn/goji/web/mutil"
)

func init() {
	formatter := new(prefixed.TextFormatter)
	log.SetFormatter(formatter)
}

// SetupServiceLogging sets up a consistent logging env for all services.
func SetupServiceLogging() {
	// Setup logging.
	log.SetOutput(os.Stdout)
	log.SetLevel(log.InfoLevel)
}

// HTTPLoggingMiddleware is a middleware function used for logging HTTP requests.
func HTTPLoggingMiddleware(next http.Handler) http.Handler {
	f := func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		lw := mutil.WrapWriter(w)
		next.ServeHTTP(lw, r)
		duration := time.Since(start)
		logFields := log.Fields{
			"req_method":  r.Method,
			"req_path":    r.URL.String(),
			"resp_time":   duration,
			"resp_code":   lw.Status(),
			"resp_status": http.StatusText(lw.Status()),
			"resp_size":   lw.BytesWritten(),
		}

		if lw.Status() != http.StatusOK {
			log.WithTime(start).WithFields(logFields).Error("HTTP Request")
		} else {
			log.WithTime(start).WithFields(logFields).Info("HTTP Request")
		}
	}
	return http.HandlerFunc(f)
}