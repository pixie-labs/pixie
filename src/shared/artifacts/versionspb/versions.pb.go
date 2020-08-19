// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: src/shared/artifacts/versionspb/versions.proto

package versionspb

import (
	fmt "fmt"
	_ "github.com/gogo/protobuf/gogoproto"
	proto "github.com/gogo/protobuf/proto"
	types "github.com/gogo/protobuf/types"
	io "io"
	math "math"
	math_bits "math/bits"
	reflect "reflect"
	strconv "strconv"
	strings "strings"
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

type ArtifactType int32

const (
	AT_UNKNOWN                   ArtifactType = 0
	AT_LINUX_AMD64               ArtifactType = 1
	AT_DARWIN_AMD64              ArtifactType = 2
	AT_CONTAINER_SET_YAMLS       ArtifactType = 50
	AT_CONTAINER_SET_LINUX_AMD64 ArtifactType = 100
)

var ArtifactType_name = map[int32]string{
	0:   "AT_UNKNOWN",
	1:   "AT_LINUX_AMD64",
	2:   "AT_DARWIN_AMD64",
	50:  "AT_CONTAINER_SET_YAMLS",
	100: "AT_CONTAINER_SET_LINUX_AMD64",
}

var ArtifactType_value = map[string]int32{
	"AT_UNKNOWN":                   0,
	"AT_LINUX_AMD64":               1,
	"AT_DARWIN_AMD64":              2,
	"AT_CONTAINER_SET_YAMLS":       50,
	"AT_CONTAINER_SET_LINUX_AMD64": 100,
}

func (ArtifactType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_11101fe785e211c4, []int{0}
}

type ArtifactSet struct {
	Name     string      `protobuf:"bytes,1,opt,name=name,proto3" json:"name,omitempty"`
	Artifact []*Artifact `protobuf:"bytes,2,rep,name=artifact,proto3" json:"artifact,omitempty"`
}

func (m *ArtifactSet) Reset()      { *m = ArtifactSet{} }
func (*ArtifactSet) ProtoMessage() {}
func (*ArtifactSet) Descriptor() ([]byte, []int) {
	return fileDescriptor_11101fe785e211c4, []int{0}
}
func (m *ArtifactSet) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *ArtifactSet) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_ArtifactSet.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *ArtifactSet) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ArtifactSet.Merge(m, src)
}
func (m *ArtifactSet) XXX_Size() int {
	return m.Size()
}
func (m *ArtifactSet) XXX_DiscardUnknown() {
	xxx_messageInfo_ArtifactSet.DiscardUnknown(m)
}

var xxx_messageInfo_ArtifactSet proto.InternalMessageInfo

func (m *ArtifactSet) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

func (m *ArtifactSet) GetArtifact() []*Artifact {
	if m != nil {
		return m.Artifact
	}
	return nil
}

type Artifact struct {
	Timestamp          *types.Timestamp `protobuf:"bytes,1,opt,name=timestamp,proto3" json:"timestamp,omitempty"`
	CommitHash         string           `protobuf:"bytes,2,opt,name=commit_hash,json=commitHash,proto3" json:"commit_hash,omitempty"`
	VersionStr         string           `protobuf:"bytes,3,opt,name=version_str,json=versionStr,proto3" json:"version_str,omitempty"`
	AvailableArtifacts []ArtifactType   `protobuf:"varint,4,rep,packed,name=available_artifacts,json=availableArtifacts,proto3,enum=pl.versions.ArtifactType" json:"available_artifacts,omitempty"`
	Changelog          string           `protobuf:"bytes,5,opt,name=changelog,proto3" json:"changelog,omitempty"`
}

func (m *Artifact) Reset()      { *m = Artifact{} }
func (*Artifact) ProtoMessage() {}
func (*Artifact) Descriptor() ([]byte, []int) {
	return fileDescriptor_11101fe785e211c4, []int{1}
}
func (m *Artifact) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *Artifact) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_Artifact.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *Artifact) XXX_Merge(src proto.Message) {
	xxx_messageInfo_Artifact.Merge(m, src)
}
func (m *Artifact) XXX_Size() int {
	return m.Size()
}
func (m *Artifact) XXX_DiscardUnknown() {
	xxx_messageInfo_Artifact.DiscardUnknown(m)
}

var xxx_messageInfo_Artifact proto.InternalMessageInfo

func (m *Artifact) GetTimestamp() *types.Timestamp {
	if m != nil {
		return m.Timestamp
	}
	return nil
}

func (m *Artifact) GetCommitHash() string {
	if m != nil {
		return m.CommitHash
	}
	return ""
}

func (m *Artifact) GetVersionStr() string {
	if m != nil {
		return m.VersionStr
	}
	return ""
}

func (m *Artifact) GetAvailableArtifacts() []ArtifactType {
	if m != nil {
		return m.AvailableArtifacts
	}
	return nil
}

func (m *Artifact) GetChangelog() string {
	if m != nil {
		return m.Changelog
	}
	return ""
}

func init() {
	proto.RegisterEnum("pl.versions.ArtifactType", ArtifactType_name, ArtifactType_value)
	proto.RegisterType((*ArtifactSet)(nil), "pl.versions.ArtifactSet")
	proto.RegisterType((*Artifact)(nil), "pl.versions.Artifact")
}

func init() {
	proto.RegisterFile("src/shared/artifacts/versionspb/versions.proto", fileDescriptor_11101fe785e211c4)
}

var fileDescriptor_11101fe785e211c4 = []byte{
	// 482 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x6c, 0x92, 0x41, 0x8b, 0xd3, 0x40,
	0x14, 0xc7, 0x33, 0xed, 0x2a, 0xdb, 0x89, 0xd4, 0x32, 0x8b, 0x12, 0xcb, 0x32, 0x5b, 0xf6, 0x54,
	0x04, 0x27, 0x58, 0x45, 0x04, 0x4f, 0xd1, 0x5d, 0xb4, 0xba, 0x9b, 0x85, 0x34, 0xcb, 0xaa, 0x97,
	0x61, 0x92, 0x9d, 0x4d, 0x82, 0x49, 0x27, 0x64, 0xa6, 0x8b, 0x1e, 0x04, 0x2f, 0xde, 0xfd, 0x18,
	0x7e, 0x14, 0x8f, 0x3d, 0xee, 0xd1, 0xa6, 0x17, 0x8f, 0xfd, 0x08, 0xd2, 0xa4, 0x49, 0x0a, 0x7a,
	0xfb, 0xbf, 0x7f, 0x7e, 0xef, 0xbd, 0x7f, 0x92, 0x07, 0x89, 0xcc, 0x7c, 0x53, 0x86, 0x2c, 0xe3,
	0x97, 0x26, 0xcb, 0x54, 0x74, 0xc5, 0x7c, 0x25, 0xcd, 0x6b, 0x9e, 0xc9, 0x48, 0x4c, 0x65, 0xea,
	0xd5, 0x92, 0xa4, 0x99, 0x50, 0x02, 0xe9, 0x69, 0x4c, 0x2a, 0xab, 0xff, 0x28, 0x88, 0x54, 0x38,
	0xf3, 0x88, 0x2f, 0x12, 0x33, 0x10, 0x81, 0x30, 0x0b, 0xc6, 0x9b, 0x5d, 0x15, 0x55, 0x51, 0x14,
	0xaa, 0xec, 0xed, 0x1f, 0x04, 0x42, 0x04, 0x31, 0x6f, 0x28, 0x15, 0x25, 0x5c, 0x2a, 0x96, 0xa4,
	0x25, 0x70, 0xe8, 0x42, 0xdd, 0xda, 0x64, 0x98, 0x70, 0x85, 0x10, 0xdc, 0x99, 0xb2, 0x84, 0x1b,
	0x60, 0x00, 0x86, 0x1d, 0xa7, 0xd0, 0xe8, 0x31, 0xdc, 0xad, 0x62, 0x1a, 0xad, 0x41, 0x7b, 0xa8,
	0x8f, 0xee, 0x91, 0xad, 0x48, 0xa4, 0xea, 0x77, 0x6a, 0xec, 0x70, 0x05, 0xe0, 0x6e, 0x65, 0xa3,
	0xe7, 0xb0, 0x53, 0x6f, 0x2d, 0x06, 0xeb, 0xa3, 0x3e, 0x29, 0x73, 0x91, 0x2a, 0x17, 0x71, 0x2b,
	0xc2, 0x69, 0x60, 0x74, 0x00, 0x75, 0x5f, 0x24, 0x49, 0xa4, 0x68, 0xc8, 0x64, 0x68, 0xb4, 0x8a,
	0x50, 0xb0, 0xb4, 0xde, 0x30, 0x19, 0xae, 0x81, 0x4d, 0x0c, 0x2a, 0x55, 0x66, 0xb4, 0x4b, 0x60,
	0x63, 0x4d, 0x54, 0x86, 0xde, 0xc2, 0x3d, 0x76, 0xcd, 0xa2, 0x98, 0x79, 0x31, 0xa7, 0xf5, 0xc7,
	0x36, 0x76, 0x06, 0xed, 0x61, 0x77, 0xf4, 0xe0, 0xbf, 0xaf, 0xe1, 0x7e, 0x49, 0xb9, 0x83, 0xea,
	0xae, 0xca, 0x96, 0x68, 0x1f, 0x76, 0xfc, 0x90, 0x4d, 0x03, 0x1e, 0x8b, 0xc0, 0xb8, 0x55, 0xac,
	0x6a, 0x8c, 0x87, 0xdf, 0x01, 0xbc, 0xb3, 0x3d, 0x02, 0x75, 0x21, 0xb4, 0x5c, 0x7a, 0x6e, 0xbf,
	0xb3, 0xcf, 0x2e, 0xec, 0x9e, 0x86, 0x10, 0xec, 0x5a, 0x2e, 0x3d, 0x19, 0xdb, 0xe7, 0xef, 0xa9,
	0x75, 0x7a, 0xf4, 0xec, 0x69, 0x0f, 0xa0, 0x3d, 0x78, 0xd7, 0x72, 0xe9, 0x91, 0xe5, 0x5c, 0x8c,
	0xed, 0x8d, 0xd9, 0x42, 0x7d, 0x78, 0xdf, 0x72, 0xe9, 0xab, 0x33, 0xdb, 0xb5, 0xc6, 0xf6, 0xb1,
	0x43, 0x27, 0xc7, 0x2e, 0xfd, 0x60, 0x9d, 0x9e, 0x4c, 0x7a, 0x23, 0x34, 0x80, 0xfb, 0xff, 0x3c,
	0xdb, 0x1e, 0x79, 0xf9, 0xf2, 0xeb, 0x7c, 0x81, 0xb5, 0x9b, 0x05, 0xd6, 0x56, 0x0b, 0x0c, 0xbe,
	0xe5, 0x18, 0xfc, 0xcc, 0x31, 0xf8, 0x95, 0x63, 0x30, 0xcf, 0x31, 0xf8, 0x9d, 0x63, 0xf0, 0x27,
	0xc7, 0xda, 0x2a, 0xc7, 0xe0, 0xc7, 0x12, 0x6b, 0xf3, 0x25, 0xd6, 0x6e, 0x96, 0x58, 0xfb, 0xf8,
	0x3a, 0x8d, 0x3e, 0x47, 0x3c, 0x66, 0x9e, 0x24, 0x2c, 0x32, 0xeb, 0xc2, 0x5c, 0x9f, 0xaa, 0x1f,
	0x8b, 0x59, 0x73, 0xa9, 0x54, 0x65, 0xcc, 0xff, 0xc4, 0xb3, 0xad, 0x83, 0x7d, 0xd1, 0x48, 0xef,
	0x76, 0xf1, 0x47, 0x9f, 0xfc, 0x0d, 0x00, 0x00, 0xff, 0xff, 0xb9, 0x5f, 0x1b, 0x33, 0xe5, 0x02,
	0x00, 0x00,
}

func (x ArtifactType) String() string {
	s, ok := ArtifactType_name[int32(x)]
	if ok {
		return s
	}
	return strconv.Itoa(int(x))
}
func (this *ArtifactSet) Equal(that interface{}) bool {
	if that == nil {
		return this == nil
	}

	that1, ok := that.(*ArtifactSet)
	if !ok {
		that2, ok := that.(ArtifactSet)
		if ok {
			that1 = &that2
		} else {
			return false
		}
	}
	if that1 == nil {
		return this == nil
	} else if this == nil {
		return false
	}
	if this.Name != that1.Name {
		return false
	}
	if len(this.Artifact) != len(that1.Artifact) {
		return false
	}
	for i := range this.Artifact {
		if !this.Artifact[i].Equal(that1.Artifact[i]) {
			return false
		}
	}
	return true
}
func (this *Artifact) Equal(that interface{}) bool {
	if that == nil {
		return this == nil
	}

	that1, ok := that.(*Artifact)
	if !ok {
		that2, ok := that.(Artifact)
		if ok {
			that1 = &that2
		} else {
			return false
		}
	}
	if that1 == nil {
		return this == nil
	} else if this == nil {
		return false
	}
	if !this.Timestamp.Equal(that1.Timestamp) {
		return false
	}
	if this.CommitHash != that1.CommitHash {
		return false
	}
	if this.VersionStr != that1.VersionStr {
		return false
	}
	if len(this.AvailableArtifacts) != len(that1.AvailableArtifacts) {
		return false
	}
	for i := range this.AvailableArtifacts {
		if this.AvailableArtifacts[i] != that1.AvailableArtifacts[i] {
			return false
		}
	}
	if this.Changelog != that1.Changelog {
		return false
	}
	return true
}
func (this *ArtifactSet) GoString() string {
	if this == nil {
		return "nil"
	}
	s := make([]string, 0, 6)
	s = append(s, "&versionspb.ArtifactSet{")
	s = append(s, "Name: "+fmt.Sprintf("%#v", this.Name)+",\n")
	if this.Artifact != nil {
		s = append(s, "Artifact: "+fmt.Sprintf("%#v", this.Artifact)+",\n")
	}
	s = append(s, "}")
	return strings.Join(s, "")
}
func (this *Artifact) GoString() string {
	if this == nil {
		return "nil"
	}
	s := make([]string, 0, 9)
	s = append(s, "&versionspb.Artifact{")
	if this.Timestamp != nil {
		s = append(s, "Timestamp: "+fmt.Sprintf("%#v", this.Timestamp)+",\n")
	}
	s = append(s, "CommitHash: "+fmt.Sprintf("%#v", this.CommitHash)+",\n")
	s = append(s, "VersionStr: "+fmt.Sprintf("%#v", this.VersionStr)+",\n")
	s = append(s, "AvailableArtifacts: "+fmt.Sprintf("%#v", this.AvailableArtifacts)+",\n")
	s = append(s, "Changelog: "+fmt.Sprintf("%#v", this.Changelog)+",\n")
	s = append(s, "}")
	return strings.Join(s, "")
}
func valueToGoStringVersions(v interface{}, typ string) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("func(v %v) *%v { return &v } ( %#v )", typ, typ, pv)
}
func (m *ArtifactSet) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *ArtifactSet) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *ArtifactSet) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if len(m.Artifact) > 0 {
		for iNdEx := len(m.Artifact) - 1; iNdEx >= 0; iNdEx-- {
			{
				size, err := m.Artifact[iNdEx].MarshalToSizedBuffer(dAtA[:i])
				if err != nil {
					return 0, err
				}
				i -= size
				i = encodeVarintVersions(dAtA, i, uint64(size))
			}
			i--
			dAtA[i] = 0x12
		}
	}
	if len(m.Name) > 0 {
		i -= len(m.Name)
		copy(dAtA[i:], m.Name)
		i = encodeVarintVersions(dAtA, i, uint64(len(m.Name)))
		i--
		dAtA[i] = 0xa
	}
	return len(dAtA) - i, nil
}

func (m *Artifact) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Artifact) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *Artifact) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if len(m.Changelog) > 0 {
		i -= len(m.Changelog)
		copy(dAtA[i:], m.Changelog)
		i = encodeVarintVersions(dAtA, i, uint64(len(m.Changelog)))
		i--
		dAtA[i] = 0x2a
	}
	if len(m.AvailableArtifacts) > 0 {
		dAtA2 := make([]byte, len(m.AvailableArtifacts)*10)
		var j1 int
		for _, num := range m.AvailableArtifacts {
			for num >= 1<<7 {
				dAtA2[j1] = uint8(uint64(num)&0x7f | 0x80)
				num >>= 7
				j1++
			}
			dAtA2[j1] = uint8(num)
			j1++
		}
		i -= j1
		copy(dAtA[i:], dAtA2[:j1])
		i = encodeVarintVersions(dAtA, i, uint64(j1))
		i--
		dAtA[i] = 0x22
	}
	if len(m.VersionStr) > 0 {
		i -= len(m.VersionStr)
		copy(dAtA[i:], m.VersionStr)
		i = encodeVarintVersions(dAtA, i, uint64(len(m.VersionStr)))
		i--
		dAtA[i] = 0x1a
	}
	if len(m.CommitHash) > 0 {
		i -= len(m.CommitHash)
		copy(dAtA[i:], m.CommitHash)
		i = encodeVarintVersions(dAtA, i, uint64(len(m.CommitHash)))
		i--
		dAtA[i] = 0x12
	}
	if m.Timestamp != nil {
		{
			size, err := m.Timestamp.MarshalToSizedBuffer(dAtA[:i])
			if err != nil {
				return 0, err
			}
			i -= size
			i = encodeVarintVersions(dAtA, i, uint64(size))
		}
		i--
		dAtA[i] = 0xa
	}
	return len(dAtA) - i, nil
}

func encodeVarintVersions(dAtA []byte, offset int, v uint64) int {
	offset -= sovVersions(v)
	base := offset
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return base
}
func (m *ArtifactSet) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	l = len(m.Name)
	if l > 0 {
		n += 1 + l + sovVersions(uint64(l))
	}
	if len(m.Artifact) > 0 {
		for _, e := range m.Artifact {
			l = e.Size()
			n += 1 + l + sovVersions(uint64(l))
		}
	}
	return n
}

func (m *Artifact) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	if m.Timestamp != nil {
		l = m.Timestamp.Size()
		n += 1 + l + sovVersions(uint64(l))
	}
	l = len(m.CommitHash)
	if l > 0 {
		n += 1 + l + sovVersions(uint64(l))
	}
	l = len(m.VersionStr)
	if l > 0 {
		n += 1 + l + sovVersions(uint64(l))
	}
	if len(m.AvailableArtifacts) > 0 {
		l = 0
		for _, e := range m.AvailableArtifacts {
			l += sovVersions(uint64(e))
		}
		n += 1 + sovVersions(uint64(l)) + l
	}
	l = len(m.Changelog)
	if l > 0 {
		n += 1 + l + sovVersions(uint64(l))
	}
	return n
}

func sovVersions(x uint64) (n int) {
	return (math_bits.Len64(x|1) + 6) / 7
}
func sozVersions(x uint64) (n int) {
	return sovVersions(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (this *ArtifactSet) String() string {
	if this == nil {
		return "nil"
	}
	repeatedStringForArtifact := "[]*Artifact{"
	for _, f := range this.Artifact {
		repeatedStringForArtifact += strings.Replace(f.String(), "Artifact", "Artifact", 1) + ","
	}
	repeatedStringForArtifact += "}"
	s := strings.Join([]string{`&ArtifactSet{`,
		`Name:` + fmt.Sprintf("%v", this.Name) + `,`,
		`Artifact:` + repeatedStringForArtifact + `,`,
		`}`,
	}, "")
	return s
}
func (this *Artifact) String() string {
	if this == nil {
		return "nil"
	}
	s := strings.Join([]string{`&Artifact{`,
		`Timestamp:` + strings.Replace(fmt.Sprintf("%v", this.Timestamp), "Timestamp", "types.Timestamp", 1) + `,`,
		`CommitHash:` + fmt.Sprintf("%v", this.CommitHash) + `,`,
		`VersionStr:` + fmt.Sprintf("%v", this.VersionStr) + `,`,
		`AvailableArtifacts:` + fmt.Sprintf("%v", this.AvailableArtifacts) + `,`,
		`Changelog:` + fmt.Sprintf("%v", this.Changelog) + `,`,
		`}`,
	}, "")
	return s
}
func valueToStringVersions(v interface{}) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("*%v", pv)
}
func (m *ArtifactSet) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVersions
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= uint64(b&0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: ArtifactSet: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: ArtifactSet: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Name", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Name = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Artifact", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= int(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + msglen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Artifact = append(m.Artifact, &Artifact{})
			if err := m.Artifact[len(m.Artifact)-1].Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVersions(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVersions
			}
			if (iNdEx + skippy) < 0 {
				return ErrInvalidLengthVersions
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func (m *Artifact) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowVersions
			}
			if iNdEx >= l {
				return io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= uint64(b&0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		fieldNum := int32(wire >> 3)
		wireType := int(wire & 0x7)
		if wireType == 4 {
			return fmt.Errorf("proto: Artifact: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Artifact: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Timestamp", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				msglen |= int(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if msglen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + msglen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.Timestamp == nil {
				m.Timestamp = &types.Timestamp{}
			}
			if err := m.Timestamp.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field CommitHash", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.CommitHash = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field VersionStr", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.VersionStr = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 4:
			if wireType == 0 {
				var v ArtifactType
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowVersions
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					v |= ArtifactType(b&0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				m.AvailableArtifacts = append(m.AvailableArtifacts, v)
			} else if wireType == 2 {
				var packedLen int
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return ErrIntOverflowVersions
					}
					if iNdEx >= l {
						return io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					packedLen |= int(b&0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				if packedLen < 0 {
					return ErrInvalidLengthVersions
				}
				postIndex := iNdEx + packedLen
				if postIndex < 0 {
					return ErrInvalidLengthVersions
				}
				if postIndex > l {
					return io.ErrUnexpectedEOF
				}
				var elementCount int
				if elementCount != 0 && len(m.AvailableArtifacts) == 0 {
					m.AvailableArtifacts = make([]ArtifactType, 0, elementCount)
				}
				for iNdEx < postIndex {
					var v ArtifactType
					for shift := uint(0); ; shift += 7 {
						if shift >= 64 {
							return ErrIntOverflowVersions
						}
						if iNdEx >= l {
							return io.ErrUnexpectedEOF
						}
						b := dAtA[iNdEx]
						iNdEx++
						v |= ArtifactType(b&0x7F) << shift
						if b < 0x80 {
							break
						}
					}
					m.AvailableArtifacts = append(m.AvailableArtifacts, v)
				}
			} else {
				return fmt.Errorf("proto: wrong wireType = %d for field AvailableArtifacts", wireType)
			}
		case 5:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Changelog", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				stringLen |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			intStringLen := int(stringLen)
			if intStringLen < 0 {
				return ErrInvalidLengthVersions
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthVersions
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Changelog = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipVersions(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthVersions
			}
			if (iNdEx + skippy) < 0 {
				return ErrInvalidLengthVersions
			}
			if (iNdEx + skippy) > l {
				return io.ErrUnexpectedEOF
			}
			iNdEx += skippy
		}
	}

	if iNdEx > l {
		return io.ErrUnexpectedEOF
	}
	return nil
}
func skipVersions(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	depth := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowVersions
			}
			if iNdEx >= l {
				return 0, io.ErrUnexpectedEOF
			}
			b := dAtA[iNdEx]
			iNdEx++
			wire |= (uint64(b) & 0x7F) << shift
			if b < 0x80 {
				break
			}
		}
		wireType := int(wire & 0x7)
		switch wireType {
		case 0:
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				iNdEx++
				if dAtA[iNdEx-1] < 0x80 {
					break
				}
			}
		case 1:
			iNdEx += 8
		case 2:
			var length int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowVersions
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				length |= (int(b) & 0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if length < 0 {
				return 0, ErrInvalidLengthVersions
			}
			iNdEx += length
		case 3:
			depth++
		case 4:
			if depth == 0 {
				return 0, ErrUnexpectedEndOfGroupVersions
			}
			depth--
		case 5:
			iNdEx += 4
		default:
			return 0, fmt.Errorf("proto: illegal wireType %d", wireType)
		}
		if iNdEx < 0 {
			return 0, ErrInvalidLengthVersions
		}
		if depth == 0 {
			return iNdEx, nil
		}
	}
	return 0, io.ErrUnexpectedEOF
}

var (
	ErrInvalidLengthVersions        = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowVersions          = fmt.Errorf("proto: integer overflow")
	ErrUnexpectedEndOfGroupVersions = fmt.Errorf("proto: unexpected end of group")
)