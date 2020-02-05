// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: src/common/base/proto/status.proto

package statuspb

import (
	fmt "fmt"
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

type Code int32

const (
	OK                   Code = 0
	CANCELLED            Code = 1
	UNKNOWN              Code = 2
	INVALID_ARGUMENT     Code = 3
	DEADLINE_EXCEEDED    Code = 4
	NOT_FOUND            Code = 5
	ALREADY_EXISTS       Code = 6
	PERMISSION_DENIED    Code = 7
	UNAUTHENTICATED      Code = 8
	INTERNAL             Code = 9
	UNIMPLEMENTED        Code = 10
	RESOURCE_UNAVAILABLE Code = 11
	SYSTEM               Code = 12
	DO_NOT_USE_          Code = 100
)

var Code_name = map[int32]string{
	0:   "OK",
	1:   "CANCELLED",
	2:   "UNKNOWN",
	3:   "INVALID_ARGUMENT",
	4:   "DEADLINE_EXCEEDED",
	5:   "NOT_FOUND",
	6:   "ALREADY_EXISTS",
	7:   "PERMISSION_DENIED",
	8:   "UNAUTHENTICATED",
	9:   "INTERNAL",
	10:  "UNIMPLEMENTED",
	11:  "RESOURCE_UNAVAILABLE",
	12:  "SYSTEM",
	100: "DO_NOT_USE_",
}

var Code_value = map[string]int32{
	"OK":                   0,
	"CANCELLED":            1,
	"UNKNOWN":              2,
	"INVALID_ARGUMENT":     3,
	"DEADLINE_EXCEEDED":    4,
	"NOT_FOUND":            5,
	"ALREADY_EXISTS":       6,
	"PERMISSION_DENIED":    7,
	"UNAUTHENTICATED":      8,
	"INTERNAL":             9,
	"UNIMPLEMENTED":        10,
	"RESOURCE_UNAVAILABLE": 11,
	"SYSTEM":               12,
	"DO_NOT_USE_":          100,
}

func (Code) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_347acecbef888913, []int{0}
}

type Status struct {
	ErrCode Code       `protobuf:"varint,1,opt,name=err_code,json=errCode,proto3,enum=pl.statuspb.Code" json:"err_code,omitempty"`
	Msg     string     `protobuf:"bytes,2,opt,name=msg,proto3" json:"msg,omitempty"`
	Context *types.Any `protobuf:"bytes,3,opt,name=context,proto3" json:"context,omitempty"`
}

func (m *Status) Reset()      { *m = Status{} }
func (*Status) ProtoMessage() {}
func (*Status) Descriptor() ([]byte, []int) {
	return fileDescriptor_347acecbef888913, []int{0}
}
func (m *Status) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *Status) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_Status.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *Status) XXX_Merge(src proto.Message) {
	xxx_messageInfo_Status.Merge(m, src)
}
func (m *Status) XXX_Size() int {
	return m.Size()
}
func (m *Status) XXX_DiscardUnknown() {
	xxx_messageInfo_Status.DiscardUnknown(m)
}

var xxx_messageInfo_Status proto.InternalMessageInfo

func (m *Status) GetErrCode() Code {
	if m != nil {
		return m.ErrCode
	}
	return OK
}

func (m *Status) GetMsg() string {
	if m != nil {
		return m.Msg
	}
	return ""
}

func (m *Status) GetContext() *types.Any {
	if m != nil {
		return m.Context
	}
	return nil
}

func init() {
	proto.RegisterEnum("pl.statuspb.Code", Code_name, Code_value)
	proto.RegisterType((*Status)(nil), "pl.statuspb.Status")
}

func init() { proto.RegisterFile("src/common/base/proto/status.proto", fileDescriptor_347acecbef888913) }

var fileDescriptor_347acecbef888913 = []byte{
	// 451 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x4c, 0x91, 0xcb, 0x6e, 0xd3, 0x40,
	0x14, 0x86, 0x3d, 0x49, 0xc9, 0xe5, 0xb8, 0x97, 0xc9, 0x10, 0xa4, 0xc0, 0x62, 0x14, 0x75, 0x15,
	0x21, 0x64, 0x4b, 0x65, 0x8f, 0x34, 0xf5, 0x1c, 0x60, 0x54, 0x67, 0x5c, 0xf9, 0x52, 0x5a, 0x36,
	0x56, 0x2e, 0x26, 0x9b, 0x26, 0x8e, 0x6c, 0x57, 0x6a, 0x77, 0x3c, 0x02, 0x8f, 0xc1, 0xa3, 0xb0,
	0xcc, 0xb2, 0x4b, 0xe2, 0x6c, 0x58, 0xf6, 0x09, 0x10, 0x72, 0x42, 0xa4, 0xee, 0xfe, 0x19, 0x7d,
	0xe7, 0xff, 0x7f, 0x9d, 0x03, 0xa7, 0x79, 0x36, 0xb1, 0x27, 0xe9, 0x7c, 0x9e, 0x2e, 0xec, 0xf1,
	0x28, 0x4f, 0xec, 0x65, 0x96, 0x16, 0xa9, 0x9d, 0x17, 0xa3, 0xe2, 0x2e, 0xb7, 0xb6, 0x0f, 0x66,
	0x2e, 0x6f, 0xad, 0xdd, 0xc7, 0x72, 0xfc, 0xe6, 0xf5, 0x2c, 0x4d, 0x67, 0xb7, 0xff, 0xb9, 0xf1,
	0xdd, 0x37, 0x7b, 0xb4, 0x78, 0xd8, 0x71, 0xa7, 0xf7, 0xd0, 0x08, 0xb6, 0x18, 0x7b, 0x07, 0xad,
	0x24, 0xcb, 0xe2, 0x49, 0x3a, 0x4d, 0x7a, 0xa4, 0x4f, 0x06, 0xc7, 0x67, 0x1d, 0xeb, 0x99, 0x89,
	0xe5, 0xa4, 0xd3, 0xc4, 0x6f, 0x26, 0x59, 0x56, 0x09, 0x46, 0xa1, 0x3e, 0xcf, 0x67, 0xbd, 0x5a,
	0x9f, 0x0c, 0xda, 0x7e, 0x25, 0x99, 0x05, 0xcd, 0x49, 0xba, 0x28, 0x92, 0xfb, 0xa2, 0x57, 0xef,
	0x93, 0x81, 0x79, 0xd6, 0xb5, 0x76, 0xb1, 0xd6, 0x3e, 0xd6, 0x12, 0x8b, 0x07, 0x7f, 0x0f, 0xbd,
	0xfd, 0x4b, 0xe0, 0x60, 0x6b, 0xd5, 0x80, 0x9a, 0x77, 0x41, 0x0d, 0x76, 0x04, 0x6d, 0x47, 0x68,
	0x07, 0x5d, 0x17, 0x25, 0x25, 0xcc, 0x84, 0x66, 0xa4, 0x2f, 0xb4, 0xf7, 0x45, 0xd3, 0x1a, 0xeb,
	0x02, 0x55, 0xfa, 0x4a, 0xb8, 0x4a, 0xc6, 0xc2, 0xff, 0x14, 0x0d, 0x51, 0x87, 0xb4, 0xce, 0x5e,
	0x41, 0x47, 0xa2, 0x90, 0xae, 0xd2, 0x18, 0xe3, 0xb5, 0x83, 0x28, 0x51, 0xd2, 0x83, 0xca, 0x48,
	0x7b, 0x61, 0xfc, 0xd1, 0x8b, 0xb4, 0xa4, 0x2f, 0x18, 0x83, 0x63, 0xe1, 0xfa, 0x28, 0xe4, 0x4d,
	0x8c, 0xd7, 0x2a, 0x08, 0x03, 0xda, 0xa8, 0x26, 0x2f, 0xd1, 0x1f, 0xaa, 0x20, 0x50, 0x9e, 0x8e,
	0x25, 0x6a, 0x85, 0x92, 0x36, 0xd9, 0x4b, 0x38, 0x89, 0xb4, 0x88, 0xc2, 0xcf, 0xa8, 0x43, 0xe5,
	0x88, 0x10, 0x25, 0x6d, 0xb1, 0x43, 0x68, 0x29, 0x1d, 0xa2, 0xaf, 0x85, 0x4b, 0xdb, 0xac, 0x03,
	0x47, 0x91, 0x56, 0xc3, 0x4b, 0x17, 0xab, 0x12, 0x28, 0x29, 0xb0, 0x1e, 0x74, 0x7d, 0x0c, 0xbc,
	0xc8, 0x77, 0x30, 0x8e, 0xb4, 0xb8, 0x12, 0xca, 0x15, 0xe7, 0x2e, 0x52, 0x93, 0x01, 0x34, 0x82,
	0x9b, 0x20, 0xc4, 0x21, 0x3d, 0x64, 0x27, 0x60, 0x4a, 0x2f, 0xae, 0x8a, 0x45, 0x01, 0xc6, 0x74,
	0x7a, 0xfe, 0x61, 0xb5, 0xe6, 0xc6, 0xe3, 0x9a, 0x1b, 0x4f, 0x6b, 0x4e, 0xbe, 0x97, 0x9c, 0xfc,
	0x2c, 0x39, 0xf9, 0x55, 0x72, 0xb2, 0x2a, 0x39, 0xf9, 0x5d, 0x72, 0xf2, 0xa7, 0xe4, 0xc6, 0x53,
	0xc9, 0xc9, 0x8f, 0x0d, 0x37, 0x56, 0x1b, 0x6e, 0x3c, 0x6e, 0xb8, 0xf1, 0xb5, 0xb5, 0x3f, 0xc8,
	0xb8, 0xb1, 0xdd, 0xeb, 0xfb, 0x7f, 0x01, 0x00, 0x00, 0xff, 0xff, 0x13, 0xee, 0x83, 0xf8, 0x0f,
	0x02, 0x00, 0x00,
}

func (x Code) String() string {
	s, ok := Code_name[int32(x)]
	if ok {
		return s
	}
	return strconv.Itoa(int(x))
}
func (this *Status) Equal(that interface{}) bool {
	if that == nil {
		return this == nil
	}

	that1, ok := that.(*Status)
	if !ok {
		that2, ok := that.(Status)
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
	if this.ErrCode != that1.ErrCode {
		return false
	}
	if this.Msg != that1.Msg {
		return false
	}
	if !this.Context.Equal(that1.Context) {
		return false
	}
	return true
}
func (this *Status) GoString() string {
	if this == nil {
		return "nil"
	}
	s := make([]string, 0, 7)
	s = append(s, "&statuspb.Status{")
	s = append(s, "ErrCode: "+fmt.Sprintf("%#v", this.ErrCode)+",\n")
	s = append(s, "Msg: "+fmt.Sprintf("%#v", this.Msg)+",\n")
	if this.Context != nil {
		s = append(s, "Context: "+fmt.Sprintf("%#v", this.Context)+",\n")
	}
	s = append(s, "}")
	return strings.Join(s, "")
}
func valueToGoStringStatus(v interface{}, typ string) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("func(v %v) *%v { return &v } ( %#v )", typ, typ, pv)
}
func (m *Status) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *Status) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *Status) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if m.Context != nil {
		{
			size, err := m.Context.MarshalToSizedBuffer(dAtA[:i])
			if err != nil {
				return 0, err
			}
			i -= size
			i = encodeVarintStatus(dAtA, i, uint64(size))
		}
		i--
		dAtA[i] = 0x1a
	}
	if len(m.Msg) > 0 {
		i -= len(m.Msg)
		copy(dAtA[i:], m.Msg)
		i = encodeVarintStatus(dAtA, i, uint64(len(m.Msg)))
		i--
		dAtA[i] = 0x12
	}
	if m.ErrCode != 0 {
		i = encodeVarintStatus(dAtA, i, uint64(m.ErrCode))
		i--
		dAtA[i] = 0x8
	}
	return len(dAtA) - i, nil
}

func encodeVarintStatus(dAtA []byte, offset int, v uint64) int {
	offset -= sovStatus(v)
	base := offset
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return base
}
func (m *Status) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	if m.ErrCode != 0 {
		n += 1 + sovStatus(uint64(m.ErrCode))
	}
	l = len(m.Msg)
	if l > 0 {
		n += 1 + l + sovStatus(uint64(l))
	}
	if m.Context != nil {
		l = m.Context.Size()
		n += 1 + l + sovStatus(uint64(l))
	}
	return n
}

func sovStatus(x uint64) (n int) {
	return (math_bits.Len64(x|1) + 6) / 7
}
func sozStatus(x uint64) (n int) {
	return sovStatus(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (this *Status) String() string {
	if this == nil {
		return "nil"
	}
	s := strings.Join([]string{`&Status{`,
		`ErrCode:` + fmt.Sprintf("%v", this.ErrCode) + `,`,
		`Msg:` + fmt.Sprintf("%v", this.Msg) + `,`,
		`Context:` + strings.Replace(fmt.Sprintf("%v", this.Context), "Any", "types.Any", 1) + `,`,
		`}`,
	}, "")
	return s
}
func valueToStringStatus(v interface{}) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("*%v", pv)
}
func (m *Status) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowStatus
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
			return fmt.Errorf("proto: Status: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: Status: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field ErrCode", wireType)
			}
			m.ErrCode = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowStatus
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.ErrCode |= Code(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 2:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Msg", wireType)
			}
			var stringLen uint64
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowStatus
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
				return ErrInvalidLengthStatus
			}
			postIndex := iNdEx + intStringLen
			if postIndex < 0 {
				return ErrInvalidLengthStatus
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.Msg = string(dAtA[iNdEx:postIndex])
			iNdEx = postIndex
		case 3:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field Context", wireType)
			}
			var msglen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowStatus
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
				return ErrInvalidLengthStatus
			}
			postIndex := iNdEx + msglen
			if postIndex < 0 {
				return ErrInvalidLengthStatus
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			if m.Context == nil {
				m.Context = &types.Any{}
			}
			if err := m.Context.Unmarshal(dAtA[iNdEx:postIndex]); err != nil {
				return err
			}
			iNdEx = postIndex
		default:
			iNdEx = preIndex
			skippy, err := skipStatus(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if skippy < 0 {
				return ErrInvalidLengthStatus
			}
			if (iNdEx + skippy) < 0 {
				return ErrInvalidLengthStatus
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
func skipStatus(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowStatus
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
					return 0, ErrIntOverflowStatus
				}
				if iNdEx >= l {
					return 0, io.ErrUnexpectedEOF
				}
				iNdEx++
				if dAtA[iNdEx-1] < 0x80 {
					break
				}
			}
			return iNdEx, nil
		case 1:
			iNdEx += 8
			return iNdEx, nil
		case 2:
			var length int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return 0, ErrIntOverflowStatus
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
				return 0, ErrInvalidLengthStatus
			}
			iNdEx += length
			if iNdEx < 0 {
				return 0, ErrInvalidLengthStatus
			}
			return iNdEx, nil
		case 3:
			for {
				var innerWire uint64
				var start int = iNdEx
				for shift := uint(0); ; shift += 7 {
					if shift >= 64 {
						return 0, ErrIntOverflowStatus
					}
					if iNdEx >= l {
						return 0, io.ErrUnexpectedEOF
					}
					b := dAtA[iNdEx]
					iNdEx++
					innerWire |= (uint64(b) & 0x7F) << shift
					if b < 0x80 {
						break
					}
				}
				innerWireType := int(innerWire & 0x7)
				if innerWireType == 4 {
					break
				}
				next, err := skipStatus(dAtA[start:])
				if err != nil {
					return 0, err
				}
				iNdEx = start + next
				if iNdEx < 0 {
					return 0, ErrInvalidLengthStatus
				}
			}
			return iNdEx, nil
		case 4:
			return iNdEx, nil
		case 5:
			iNdEx += 4
			return iNdEx, nil
		default:
			return 0, fmt.Errorf("proto: illegal wireType %d", wireType)
		}
	}
	panic("unreachable")
}

var (
	ErrInvalidLengthStatus = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowStatus   = fmt.Errorf("proto: integer overflow")
)
