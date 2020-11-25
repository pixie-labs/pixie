#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "src/common/base/inet_utils.h"
#include "src/stirling/protocols/dns/dns_stitcher.h"

using ::testing::HasSubstr;
using ::testing::IsEmpty;

namespace pl {
namespace stirling {
namespace protocols {
namespace dns {

//-----------------------------------------------------------------------------
// Test Utils
//-----------------------------------------------------------------------------

DNSHeader header;
std::vector<DNSRecord> records;

Frame CreateReqFrame(uint64_t timestamp_ns, uint16_t txid) {
  Frame f;
  f.header.txid = txid;
  f.header.flags = 0x0100;  // Standard query.
  f.header.num_queries = 1;
  f.header.num_answers = 0;
  f.header.num_auth = 0;
  f.header.num_addl = 0;
  f.records = {};
  f.timestamp_ns = timestamp_ns;
  return f;
}

Frame CreateRespFrame(uint64_t timestamp_ns, uint16_t txid, std::vector<DNSRecord> records) {
  Frame f;
  f.header.txid = txid;
  f.header.flags = 0x8180;  // Standard query response, No error
  f.header.num_queries = 1;
  f.header.num_answers = records.size();
  f.header.num_auth = 0;
  f.header.num_addl = 0;
  f.records = std::move(records);
  f.timestamp_ns = timestamp_ns;

  return f;
}

//-----------------------------------------------------------------------------
// Test Cases
//-----------------------------------------------------------------------------

TEST(DnsStitcherTest, RecordOutput) {
  std::deque<Frame> req_frames;
  std::deque<Frame> resp_frames;
  RecordsWithErrorCount<Record> result;

  InetAddr ip_addr;
  ip_addr.family = InetAddrFamily::kIPv4;
  struct in_addr addr_tmp;
  PL_CHECK_OK(ParseIPv4Addr("1.2.3.4", &addr_tmp));
  ip_addr.addr = addr_tmp;

  std::vector<DNSRecord> dns_records;
  dns_records.push_back(DNSRecord{"pixie.ai", "", ip_addr});

  int t = 0;
  Frame req0_frame = CreateReqFrame(++t, 0);
  Frame resp0_frame = CreateRespFrame(++t, 0, dns_records);

  req_frames.push_back(req0_frame);
  resp_frames.push_back(resp0_frame);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(req_frames.size(), 0);
  EXPECT_EQ(result.error_count, 0);
  ASSERT_EQ(result.records.size(), 1);

  Record& record = result.records.front();

  EXPECT_EQ(record.req.timestamp_ns, 1);
  EXPECT_EQ(record.req.header,
            R"({"txid":0,"qr":0,"opcode":0,"aa":0,"tc":0,"rd":1,"ra":0,"ad":0,"cd":0,"rcode":0,)"
            R"("num_queries":1,"num_answers":0,"num_auth":0,"num_addl":0})");
  EXPECT_EQ(record.req.query, R"({"queries":[]})");

  EXPECT_EQ(record.resp.timestamp_ns, 2);
  EXPECT_EQ(record.resp.header,
            R"({"txid":0,"qr":1,"opcode":0,"aa":0,"tc":0,"rd":1,"ra":1,"ad":0,"cd":0,"rcode":0,)"
            R"("num_queries":1,"num_answers":1,"num_auth":0,"num_addl":0})");
  EXPECT_EQ(record.resp.msg, R"({"answers":[{"name":"pixie.ai","type":"A","addr":"1.2.3.4"}]})");
}

TEST(DnsStitcherTest, OutOfOrderMatching) {
  std::deque<Frame> req_frames;
  std::deque<Frame> resp_frames;
  RecordsWithErrorCount<Record> result;

  int t = 0;

  Frame req0_frame = CreateReqFrame(++t, 0);
  Frame resp0_frame = CreateRespFrame(++t, 0, std::vector<DNSRecord>());
  Frame req1_frame = CreateReqFrame(++t, 1);
  Frame resp1_frame = CreateRespFrame(++t, 1, std::vector<DNSRecord>());
  Frame req2_frame = CreateReqFrame(++t, 2);
  Frame resp2_frame = CreateRespFrame(++t, 2, std::vector<DNSRecord>());

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(req_frames.size(), 0);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 0);

  req_frames.push_back(req0_frame);
  req_frames.push_back(req1_frame);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(req_frames.size(), 2);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 0);

  resp_frames.push_back(resp1_frame);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(req_frames.size(), 2);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 1);

  req_frames.push_back(req2_frame);
  resp_frames.push_back(resp0_frame);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(req_frames.size(), 1);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 1);

  resp_frames.push_back(resp2_frame);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(resp_frames.size(), 0);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 1);

  result = StitchFrames(&req_frames, &resp_frames);
  EXPECT_TRUE(resp_frames.empty());
  EXPECT_EQ(resp_frames.size(), 0);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records.size(), 0);
}

}  // namespace dns
}  // namespace protocols
}  // namespace stirling
}  // namespace pl