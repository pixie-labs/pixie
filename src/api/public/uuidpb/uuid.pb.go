/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Code generated by protoc-gen-gogo. DO NOT EDIT.
// source: src/api/public/uuidpb/uuid.proto

package uuidpb

import (
	bytes "bytes"
	fmt "fmt"
	proto "github.com/gogo/protobuf/proto"
	io "io"
	math "math"
	math_bits "math/bits"
	reflect "reflect"
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

type UUID struct {
	DeprecatedData []byte `protobuf:"bytes,1,opt,name=deprecated_data,json=deprecatedData,proto3" json:"deprecated_data,omitempty"`
	HighBits       uint64 `protobuf:"varint,2,opt,name=high_bits,json=highBits,proto3" json:"high_bits,omitempty"`
	LowBits        uint64 `protobuf:"varint,3,opt,name=low_bits,json=lowBits,proto3" json:"low_bits,omitempty"`
}

func (m *UUID) Reset()      { *m = UUID{} }
func (*UUID) ProtoMessage() {}
func (*UUID) Descriptor() ([]byte, []int) {
	return fileDescriptor_fb3ce78605299c5a, []int{0}
}
func (m *UUID) XXX_Unmarshal(b []byte) error {
	return m.Unmarshal(b)
}
func (m *UUID) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	if deterministic {
		return xxx_messageInfo_UUID.Marshal(b, m, deterministic)
	} else {
		b = b[:cap(b)]
		n, err := m.MarshalToSizedBuffer(b)
		if err != nil {
			return nil, err
		}
		return b[:n], nil
	}
}
func (m *UUID) XXX_Merge(src proto.Message) {
	xxx_messageInfo_UUID.Merge(m, src)
}
func (m *UUID) XXX_Size() int {
	return m.Size()
}
func (m *UUID) XXX_DiscardUnknown() {
	xxx_messageInfo_UUID.DiscardUnknown(m)
}

var xxx_messageInfo_UUID proto.InternalMessageInfo

func (m *UUID) GetDeprecatedData() []byte {
	if m != nil {
		return m.DeprecatedData
	}
	return nil
}

func (m *UUID) GetHighBits() uint64 {
	if m != nil {
		return m.HighBits
	}
	return 0
}

func (m *UUID) GetLowBits() uint64 {
	if m != nil {
		return m.LowBits
	}
	return 0
}

func init() {
	proto.RegisterType((*UUID)(nil), "pl.uuidpb.UUID")
}

func init() { proto.RegisterFile("src/api/public/uuidpb/uuid.proto", fileDescriptor_fb3ce78605299c5a) }

var fileDescriptor_fb3ce78605299c5a = []byte{
	// 239 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xe2, 0x52, 0x28, 0x2e, 0x4a, 0xd6,
	0x4f, 0x2c, 0xc8, 0xd4, 0x2f, 0x28, 0x4d, 0xca, 0xc9, 0x4c, 0xd6, 0x2f, 0x2d, 0xcd, 0x4c, 0x29,
	0x48, 0x02, 0x53, 0x7a, 0x05, 0x45, 0xf9, 0x25, 0xf9, 0x42, 0x9c, 0x05, 0x39, 0x7a, 0x10, 0x51,
	0xa5, 0x74, 0x2e, 0x96, 0xd0, 0x50, 0x4f, 0x17, 0x21, 0x75, 0x2e, 0xfe, 0x94, 0xd4, 0x82, 0xa2,
	0xd4, 0xe4, 0xc4, 0x92, 0xd4, 0x94, 0xf8, 0x94, 0xc4, 0x92, 0x44, 0x09, 0x46, 0x05, 0x46, 0x0d,
	0x9e, 0x20, 0x3e, 0x84, 0xb0, 0x4b, 0x62, 0x49, 0xa2, 0x90, 0x34, 0x17, 0x67, 0x46, 0x66, 0x7a,
	0x46, 0x7c, 0x52, 0x66, 0x49, 0xb1, 0x04, 0x93, 0x02, 0xa3, 0x06, 0x4b, 0x10, 0x07, 0x48, 0xc0,
	0x29, 0xb3, 0xa4, 0x58, 0x48, 0x92, 0x8b, 0x23, 0x27, 0xbf, 0x1c, 0x22, 0xc7, 0x0c, 0x96, 0x63,
	0xcf, 0xc9, 0x2f, 0x07, 0x49, 0x39, 0x65, 0x5e, 0x78, 0x28, 0xc7, 0x70, 0xe3, 0xa1, 0x1c, 0xc3,
	0x87, 0x87, 0x72, 0x8c, 0x0d, 0x8f, 0xe4, 0x18, 0x57, 0x3c, 0x92, 0x63, 0x3c, 0xf1, 0x48, 0x8e,
	0xf1, 0xc2, 0x23, 0x39, 0xc6, 0x07, 0x8f, 0xe4, 0x18, 0x5f, 0x3c, 0x92, 0x63, 0xf8, 0xf0, 0x48,
	0x8e, 0x71, 0xc2, 0x63, 0x39, 0x86, 0x0b, 0x8f, 0xe5, 0x18, 0x6e, 0x3c, 0x96, 0x63, 0x88, 0x32,
	0x2e, 0xc8, 0xac, 0xc8, 0x4c, 0xcd, 0x49, 0x4c, 0x2a, 0xd6, 0x4b, 0xcc, 0xd4, 0x87, 0x73, 0xf4,
	0x41, 0x5e, 0x84, 0x7a, 0x0f, 0xe4, 0x53, 0x88, 0x67, 0xac, 0x21, 0x54, 0x12, 0x1b, 0xd8, 0x97,
	0xc6, 0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xb1, 0x5e, 0xc5, 0xae, 0x09, 0x01, 0x00, 0x00,
}

func (this *UUID) Equal(that interface{}) bool {
	if that == nil {
		return this == nil
	}

	that1, ok := that.(*UUID)
	if !ok {
		that2, ok := that.(UUID)
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
	if !bytes.Equal(this.DeprecatedData, that1.DeprecatedData) {
		return false
	}
	if this.HighBits != that1.HighBits {
		return false
	}
	if this.LowBits != that1.LowBits {
		return false
	}
	return true
}
func (this *UUID) GoString() string {
	if this == nil {
		return "nil"
	}
	s := make([]string, 0, 7)
	s = append(s, "&uuidpb.UUID{")
	s = append(s, "DeprecatedData: "+fmt.Sprintf("%#v", this.DeprecatedData)+",\n")
	s = append(s, "HighBits: "+fmt.Sprintf("%#v", this.HighBits)+",\n")
	s = append(s, "LowBits: "+fmt.Sprintf("%#v", this.LowBits)+",\n")
	s = append(s, "}")
	return strings.Join(s, "")
}
func valueToGoStringUuid(v interface{}, typ string) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("func(v %v) *%v { return &v } ( %#v )", typ, typ, pv)
}
func (m *UUID) Marshal() (dAtA []byte, err error) {
	size := m.Size()
	dAtA = make([]byte, size)
	n, err := m.MarshalToSizedBuffer(dAtA[:size])
	if err != nil {
		return nil, err
	}
	return dAtA[:n], nil
}

func (m *UUID) MarshalTo(dAtA []byte) (int, error) {
	size := m.Size()
	return m.MarshalToSizedBuffer(dAtA[:size])
}

func (m *UUID) MarshalToSizedBuffer(dAtA []byte) (int, error) {
	i := len(dAtA)
	_ = i
	var l int
	_ = l
	if m.LowBits != 0 {
		i = encodeVarintUuid(dAtA, i, uint64(m.LowBits))
		i--
		dAtA[i] = 0x18
	}
	if m.HighBits != 0 {
		i = encodeVarintUuid(dAtA, i, uint64(m.HighBits))
		i--
		dAtA[i] = 0x10
	}
	if len(m.DeprecatedData) > 0 {
		i -= len(m.DeprecatedData)
		copy(dAtA[i:], m.DeprecatedData)
		i = encodeVarintUuid(dAtA, i, uint64(len(m.DeprecatedData)))
		i--
		dAtA[i] = 0xa
	}
	return len(dAtA) - i, nil
}

func encodeVarintUuid(dAtA []byte, offset int, v uint64) int {
	offset -= sovUuid(v)
	base := offset
	for v >= 1<<7 {
		dAtA[offset] = uint8(v&0x7f | 0x80)
		v >>= 7
		offset++
	}
	dAtA[offset] = uint8(v)
	return base
}
func (m *UUID) Size() (n int) {
	if m == nil {
		return 0
	}
	var l int
	_ = l
	l = len(m.DeprecatedData)
	if l > 0 {
		n += 1 + l + sovUuid(uint64(l))
	}
	if m.HighBits != 0 {
		n += 1 + sovUuid(uint64(m.HighBits))
	}
	if m.LowBits != 0 {
		n += 1 + sovUuid(uint64(m.LowBits))
	}
	return n
}

func sovUuid(x uint64) (n int) {
	return (math_bits.Len64(x|1) + 6) / 7
}
func sozUuid(x uint64) (n int) {
	return sovUuid(uint64((x << 1) ^ uint64((int64(x) >> 63))))
}
func (this *UUID) String() string {
	if this == nil {
		return "nil"
	}
	s := strings.Join([]string{`&UUID{`,
		`DeprecatedData:` + fmt.Sprintf("%v", this.DeprecatedData) + `,`,
		`HighBits:` + fmt.Sprintf("%v", this.HighBits) + `,`,
		`LowBits:` + fmt.Sprintf("%v", this.LowBits) + `,`,
		`}`,
	}, "")
	return s
}
func valueToStringUuid(v interface{}) string {
	rv := reflect.ValueOf(v)
	if rv.IsNil() {
		return "nil"
	}
	pv := reflect.Indirect(rv).Interface()
	return fmt.Sprintf("*%v", pv)
}
func (m *UUID) Unmarshal(dAtA []byte) error {
	l := len(dAtA)
	iNdEx := 0
	for iNdEx < l {
		preIndex := iNdEx
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return ErrIntOverflowUuid
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
			return fmt.Errorf("proto: UUID: wiretype end group for non-group")
		}
		if fieldNum <= 0 {
			return fmt.Errorf("proto: UUID: illegal tag %d (wire type %d)", fieldNum, wire)
		}
		switch fieldNum {
		case 1:
			if wireType != 2 {
				return fmt.Errorf("proto: wrong wireType = %d for field DeprecatedData", wireType)
			}
			var byteLen int
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowUuid
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				byteLen |= int(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
			if byteLen < 0 {
				return ErrInvalidLengthUuid
			}
			postIndex := iNdEx + byteLen
			if postIndex < 0 {
				return ErrInvalidLengthUuid
			}
			if postIndex > l {
				return io.ErrUnexpectedEOF
			}
			m.DeprecatedData = append(m.DeprecatedData[:0], dAtA[iNdEx:postIndex]...)
			if m.DeprecatedData == nil {
				m.DeprecatedData = []byte{}
			}
			iNdEx = postIndex
		case 2:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field HighBits", wireType)
			}
			m.HighBits = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowUuid
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.HighBits |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		case 3:
			if wireType != 0 {
				return fmt.Errorf("proto: wrong wireType = %d for field LowBits", wireType)
			}
			m.LowBits = 0
			for shift := uint(0); ; shift += 7 {
				if shift >= 64 {
					return ErrIntOverflowUuid
				}
				if iNdEx >= l {
					return io.ErrUnexpectedEOF
				}
				b := dAtA[iNdEx]
				iNdEx++
				m.LowBits |= uint64(b&0x7F) << shift
				if b < 0x80 {
					break
				}
			}
		default:
			iNdEx = preIndex
			skippy, err := skipUuid(dAtA[iNdEx:])
			if err != nil {
				return err
			}
			if (skippy < 0) || (iNdEx+skippy) < 0 {
				return ErrInvalidLengthUuid
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
func skipUuid(dAtA []byte) (n int, err error) {
	l := len(dAtA)
	iNdEx := 0
	depth := 0
	for iNdEx < l {
		var wire uint64
		for shift := uint(0); ; shift += 7 {
			if shift >= 64 {
				return 0, ErrIntOverflowUuid
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
					return 0, ErrIntOverflowUuid
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
					return 0, ErrIntOverflowUuid
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
				return 0, ErrInvalidLengthUuid
			}
			iNdEx += length
		case 3:
			depth++
		case 4:
			if depth == 0 {
				return 0, ErrUnexpectedEndOfGroupUuid
			}
			depth--
		case 5:
			iNdEx += 4
		default:
			return 0, fmt.Errorf("proto: illegal wireType %d", wireType)
		}
		if iNdEx < 0 {
			return 0, ErrInvalidLengthUuid
		}
		if depth == 0 {
			return iNdEx, nil
		}
	}
	return 0, io.ErrUnexpectedEOF
}

var (
	ErrInvalidLengthUuid        = fmt.Errorf("proto: negative length found during unmarshaling")
	ErrIntOverflowUuid          = fmt.Errorf("proto: integer overflow")
	ErrUnexpectedEndOfGroupUuid = fmt.Errorf("proto: unexpected end of group")
)
