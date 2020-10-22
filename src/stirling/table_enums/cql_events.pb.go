// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: src/stirling/table_enums/cql_events.proto

package cqlpb

import (
	fmt "fmt"
	proto "github.com/gogo/protobuf/proto"
	math "math"
	strconv "strconv"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.GoGoProtoPackageIsVersion3 // please upgrade the proto package

type ReqOp int32

const (
	UNUSED       ReqOp = 0
	Startup      ReqOp = 1
	AuthResponse ReqOp = 15
	Options      ReqOp = 5
	Query        ReqOp = 7
	Prepare      ReqOp = 9
	Execute      ReqOp = 10
	Batch        ReqOp = 13
	Register     ReqOp = 11
)

var ReqOp_name = map[int32]string{
	0:  "UNUSED",
	1:  "Startup",
	15: "AuthResponse",
	5:  "Options",
	7:  "Query",
	9:  "Prepare",
	10: "Execute",
	13: "Batch",
	11: "Register",
}

var ReqOp_value = map[string]int32{
	"UNUSED":       0,
	"Startup":      1,
	"AuthResponse": 15,
	"Options":      5,
	"Query":        7,
	"Prepare":      9,
	"Execute":      10,
	"Batch":        13,
	"Register":     11,
}

func (ReqOp) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_c6dd322415ca96fc, []int{0}
}

type RespOp int32

const (
	Error         RespOp = 0
	Ready         RespOp = 2
	Authenticate  RespOp = 3
	Supported     RespOp = 6
	Result        RespOp = 8
	Event         RespOp = 12
	AuthChallenge RespOp = 14
	AuthSuccess   RespOp = 16
)

var RespOp_name = map[int32]string{
	0:  "Error",
	2:  "Ready",
	3:  "Authenticate",
	6:  "Supported",
	8:  "Result",
	12: "Event",
	14: "AuthChallenge",
	16: "AuthSuccess",
}

var RespOp_value = map[string]int32{
	"Error":         0,
	"Ready":         2,
	"Authenticate":  3,
	"Supported":     6,
	"Result":        8,
	"Event":         12,
	"AuthChallenge": 14,
	"AuthSuccess":   16,
}

func (RespOp) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_c6dd322415ca96fc, []int{1}
}

func init() {
	proto.RegisterEnum("pl.stirling.tableenums.cqlpb.ReqOp", ReqOp_name, ReqOp_value)
	proto.RegisterEnum("pl.stirling.tableenums.cqlpb.RespOp", RespOp_name, RespOp_value)
}

func init() {
	proto.RegisterFile("src/stirling/table_enums/cql_events.proto", fileDescriptor_c6dd322415ca96fc)
}

var fileDescriptor_c6dd322415ca96fc = []byte{
	// 340 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x34, 0x90, 0xbf, 0x6e, 0x22, 0x31,
	0x10, 0xc6, 0xed, 0x3b, 0xed, 0x02, 0x06, 0x8e, 0x39, 0xd7, 0x27, 0xf7, 0x47, 0xb1, 0x14, 0x29,
	0x53, 0x85, 0x84, 0x36, 0x24, 0xbb, 0xa2, 0x49, 0x83, 0x16, 0x33, 0x82, 0x95, 0x9c, 0x5d, 0xe3,
	0x3f, 0x51, 0x88, 0x14, 0x29, 0x8f, 0x90, 0xc7, 0xc8, 0xa3, 0xa4, 0xa4, 0xa4, 0x0c, 0x4b, 0x93,
	0x92, 0x47, 0x88, 0xbc, 0x12, 0x9d, 0xfd, 0xcd, 0x6f, 0xbe, 0x99, 0xf9, 0xd8, 0x7f, 0x6b, 0xe4,
	0xc8, 0xba, 0xc2, 0xa8, 0xa2, 0x5c, 0x8d, 0x5c, 0xbe, 0x50, 0x38, 0xc7, 0xd2, 0x3f, 0xda, 0x91,
	0xdc, 0xa8, 0x39, 0x3e, 0x61, 0xe9, 0x6c, 0xa2, 0x4d, 0xe5, 0x2a, 0xfe, 0x4f, 0xab, 0xe4, 0x4c,
	0x26, 0x0d, 0xd9, 0x80, 0x89, 0xdc, 0x28, 0xbd, 0x18, 0xbe, 0xb2, 0x28, 0xc5, 0xcd, 0x54, 0x73,
	0xc6, 0xe2, 0xd9, 0xed, 0x2c, 0x9b, 0xdc, 0x00, 0xe1, 0x5d, 0xd6, 0xca, 0x5c, 0x6e, 0x9c, 0xd7,
	0x40, 0x39, 0xb0, 0xde, 0x95, 0x77, 0xeb, 0x14, 0xad, 0xae, 0x4a, 0x8b, 0x30, 0x08, 0xe5, 0xa9,
	0x76, 0x45, 0x55, 0x5a, 0x88, 0x78, 0x87, 0x45, 0xf7, 0x1e, 0xcd, 0x16, 0x5a, 0x41, 0xbf, 0x33,
	0xa8, 0x73, 0x83, 0xd0, 0x09, 0x9f, 0xc9, 0x33, 0x4a, 0xef, 0x10, 0x58, 0x80, 0xc6, 0xb9, 0x93,
	0x6b, 0xe8, 0xf3, 0x1e, 0x6b, 0xa7, 0xb8, 0x2a, 0xac, 0x43, 0x03, 0xdd, 0xe1, 0x0b, 0x8b, 0x83,
	0xf1, 0x54, 0x07, 0x64, 0x62, 0x4c, 0x65, 0x80, 0x84, 0x67, 0x8a, 0xf9, 0x72, 0x0b, 0xbf, 0xce,
	0xc3, 0xb1, 0x74, 0x85, 0xcc, 0x1d, 0xc2, 0x6f, 0xde, 0x67, 0x9d, 0xcc, 0x6b, 0x5d, 0x19, 0x87,
	0x4b, 0x88, 0xc3, 0xda, 0x29, 0x5a, 0xaf, 0x1c, 0xb4, 0x1b, 0x8b, 0x70, 0x39, 0xf4, 0xf8, 0x5f,
	0xd6, 0x0f, 0x7d, 0xd7, 0xeb, 0x5c, 0x29, 0x2c, 0x57, 0x08, 0x7f, 0xf8, 0x80, 0x75, 0x83, 0x94,
	0x79, 0x29, 0xd1, 0x5a, 0x80, 0xf1, 0xe5, 0xee, 0x20, 0xc8, 0xfe, 0x20, 0xc8, 0xe9, 0x20, 0xe8,
	0x5b, 0x2d, 0xe8, 0x47, 0x2d, 0xe8, 0x67, 0x2d, 0xe8, 0xae, 0x16, 0xf4, 0xab, 0x16, 0xf4, 0xbb,
	0x16, 0xe4, 0x54, 0x0b, 0xfa, 0x7e, 0x14, 0x64, 0x77, 0x14, 0x64, 0x7f, 0x14, 0xe4, 0x21, 0x6a,
	0x72, 0x5b, 0xc4, 0x4d, 0xb8, 0x17, 0x3f, 0x01, 0x00, 0x00, 0xff, 0xff, 0xc1, 0x2e, 0x53, 0xf7,
	0x89, 0x01, 0x00, 0x00,
}

func (x ReqOp) String() string {
	s, ok := ReqOp_name[int32(x)]
	if ok {
		return s
	}
	return strconv.Itoa(int(x))
}
func (x RespOp) String() string {
	s, ok := RespOp_name[int32(x)]
	if ok {
		return s
	}
	return strconv.Itoa(int(x))
}