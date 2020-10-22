package controllers

import "errors"

var (
	// ErrTracepointRegistrationFailed failed to register tracepoint.
	ErrTracepointRegistrationFailed = errors.New("failed to register tracepoints")
	// ErrTracepointDeletionFailed failed to delete tracepoint.
	ErrTracepointDeletionFailed = errors.New("failed to delete tracepoints")
	// ErrTracepointPending tracepoint is still pending.
	ErrTracepointPending = errors.New("tracepoints are still pending")
)