package controller

import (
	"context"
	"fmt"

	"pixielabs.ai/pixielabs/src/cloud/cloudapipb"
)

// CLIArtifactResolver is the resolver responsible for resolving the CLI artifact.
type CLIArtifactResolver struct {
	URL    string
	SHA256 string
}

type cliArtifactArgs struct {
	ArtifactType *string
}

func artifactTypeToProto(a *string) cloudapipb.ArtifactType {
	switch *a {
	case "AT_LINUX_AMD64":
		return cloudapipb.AT_LINUX_AMD64
	case "AT_DARWIN_AMD64":
		return cloudapipb.AT_DARWIN_AMD64
	case "AT_CONTAINER_SET_YAMLS":
		return cloudapipb.AT_CONTAINER_SET_YAMLS
	case "AT_CONTAINER_SET_LINUX_AMD64":
		return cloudapipb.AT_CONTAINER_SET_LINUX_AMD64
	default:
		return cloudapipb.AT_UNKNOWN
	}
}

// CLIArtifact resolves CLI information.
func (q *QueryResolver) CLIArtifact(ctx context.Context, args *cliArtifactArgs) (*CLIArtifactResolver, error) {
	grpcAPI := q.Env.ArtifactTrackerServer

	artifactTypePb := artifactTypeToProto(args.ArtifactType)

	artifactReq := &cloudapipb.GetArtifactListRequest{
		ArtifactType: artifactTypePb,
		ArtifactName: "cli",
		Limit:        1,
	}

	cliInfo, err := grpcAPI.GetArtifactList(ctx, artifactReq)
	if err != nil {
		return nil, err
	}

	if len(cliInfo.Artifact) == 0 {
		return nil, fmt.Errorf("No artifact exists")
	}
	if len(cliInfo.Artifact) > 1 {
		return nil, fmt.Errorf("Got unexpected number of artifacts: %d", len(cliInfo.Artifact))
	}

	linkReq := &cloudapipb.GetDownloadLinkRequest{
		ArtifactName: "cli",
		ArtifactType: artifactTypePb,
		VersionStr:   cliInfo.Artifact[0].VersionStr,
	}

	linkResp, err := grpcAPI.GetDownloadLink(ctx, linkReq)
	if err != nil {
		return nil, err
	}

	return &CLIArtifactResolver{
		linkResp.Url, linkResp.SHA256,
	}, nil
}

type artifactsArgs struct {
	ArtifactName *string
}

// ArtifactResolver is a resolver for a single artifact.
type ArtifactResolver struct {
	TimestampMs float64
	Version     string
	Changelog   string
}

// ArtifactsInfoResolver is a resolver for a list of artifacts.
type ArtifactsInfoResolver struct {
	Items *[]*ArtifactResolver
}

// Artifacts is the resolver responsible for fetching all artifacts.
func (q *QueryResolver) Artifacts(ctx context.Context, args *artifactsArgs) (*ArtifactsInfoResolver, error) {
	grpcAPI := q.Env.ArtifactTrackerServer

	artifactType := cloudapipb.AT_LINUX_AMD64
	if *args.ArtifactName == "vizier" {
		artifactType = cloudapipb.AT_CONTAINER_SET_LINUX_AMD64
	}

	artifactReq := &cloudapipb.GetArtifactListRequest{
		ArtifactType: artifactType,
		ArtifactName: *args.ArtifactName,
	}

	resp, err := grpcAPI.GetArtifactList(ctx, artifactReq)
	if err != nil {
		return nil, err
	}

	artifacts := make([]*ArtifactResolver, len(resp.Artifact))
	for i, a := range resp.Artifact {
		ts := a.Timestamp.Seconds * 1000
		artifacts[i] = &ArtifactResolver{
			Version:     a.VersionStr,
			Changelog:   a.Changelog,
			TimestampMs: float64(ts),
		}
	}
	return &ArtifactsInfoResolver{
		Items: &artifacts,
	}, nil
}