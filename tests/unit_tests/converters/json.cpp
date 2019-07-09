/**
 * \file tests/unit_tests/converters/json.cpp
 * \author Lukas Hutak <lukas.hutak@cesnet.cz>
 * \author Pavel Yadlouski <xyadlo00@stud.fit.vutbr.cz>
 * \brief IPFIX Data Record to JSON converter
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 CESNET z.s.p.o.
 */

#include <libfds.h>
#include <stdexcept>
#include <limits>
#include <gtest/gtest.h>
#include <MsgGen.h>

// Add JSON parser
#define CONFIGURU_IMPLEMENTATION 1
#include "tools/configuru.hpp"
using namespace configuru;

// Path to file with defintion of Information Elements
static const char *cfg_path = "data/iana.xml";

int
main(int argc, char **argv)
{
    // Process command line parameters and run tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

/// Base TestCase fixture
class Drec_base : public ::testing::Test {
protected:
    /// Before each Test case
    void SetUp() override {
        memset(&m_drec, 0, sizeof m_drec);

        // Load Information Elements
        m_iemgr.reset(fds_iemgr_create());
        ASSERT_NE(m_iemgr, nullptr);
        ASSERT_EQ(fds_iemgr_read_file(m_iemgr.get(), cfg_path, false), FDS_OK);

        // Create a Template Manager
        m_tmgr.reset(fds_tmgr_create(FDS_SESSION_FILE));
        ASSERT_NE(m_tmgr, nullptr);
        ASSERT_EQ(fds_tmgr_set_iemgr(m_tmgr.get(), m_iemgr.get()), FDS_OK);
        ASSERT_EQ(fds_tmgr_set_time(m_tmgr.get(), 0), FDS_OK);
    }

    /// After each Test case
    void TearDown() override {
        // Free the Data Record
        free(m_drec.data);
    }

    /**
     * @brief Add a Template to the Template manager
     * @param[in] trec Template record generator
     * @param[in] type Type of the Template
     * @warning The @p trec cannot be used after this call!
     */
    void
    register_template(ipfix_trec &trec, enum fds_template_type type = FDS_TYPE_TEMPLATE)
    {
        // Parse the Template
        uint16_t tmplt_size = trec.size();
        std::unique_ptr<uint8_t, decltype(&free)> tmptl_raw(trec.release(), &free);
        struct fds_template *tmplt_temp;
        ASSERT_EQ(fds_template_parse(type, tmptl_raw.get(), &tmplt_size, &tmplt_temp), FDS_OK);

        // Add the Template to the Template Manager
        std::unique_ptr<fds_template, decltype(&fds_template_destroy)> tmplt_wrap(tmplt_temp,
            &fds_template_destroy);
        ASSERT_EQ(fds_tmgr_template_add(m_tmgr.get(), tmplt_wrap.get()), FDS_OK);
        tmplt_wrap.release();
    }

    /**
     * @brief Create a IPFIX Data Record from a generator
     * @param[in] tid  Template ID based on which the Data Record is formatted
     * @param[in] drec Data Record generator
     * @warning The @p drec cannot be used after this call!
     */
    void
    drec_create(uint16_t tid, ipfix_drec &drec)
    {
        // Extract the Data Record
        uint16_t drec_size = drec.size();
        std::unique_ptr<uint8_t, decltype(&free)> drec_raw(drec.release(), &free);

        // Prepare a Template snapshot and Template
        ASSERT_EQ(fds_tmgr_snapshot_get(m_tmgr.get(), &m_drec.snap), FDS_OK);
        m_drec.tmplt = fds_tsnapshot_template_get(m_drec.snap, tid);
        ASSERT_NE(m_drec.tmplt, nullptr) << "Template ID not found";

        m_drec.data = drec_raw.release();
        m_drec.size = drec_size;
    }

    /// Manager of Information Elements
    std::unique_ptr<fds_iemgr_t, decltype(&fds_iemgr_destroy)> m_iemgr = {nullptr, &fds_iemgr_destroy};
    /// Template manager
    std::unique_ptr<fds_tmgr_t, decltype(&fds_tmgr_destroy)> m_tmgr = {nullptr, &fds_tmgr_destroy};
    /// IPFIX Data Record
    struct fds_drec m_drec;
};

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record of a simple flow
class Drec_basic : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  8, 4);             // sourceIPv4Address
        trec.add_field( 12, 4);             // destinationIPv4Address
        trec.add_field(  7, 2);             // sourceTransportPort
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field(  4, 1);             // protocolIdentifier
        trec.add_field(210, 3);             // -- paddingOctets
        trec.add_field(152, 8);             // flowStartMilliseconds
        trec.add_field(153, 8);             // flowEndMilliseconds
        trec.add_field(  1, 8);             // octetDeltaCount
        trec.add_field(  2, 8);             // packetDeltaCount
        trec.add_field(100, 4, 10000);      // -- field with unknown definition --
        trec.add_field(  6, 1);             // tcpControlBits

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 8);
        drec.append_float(VALUE_UNKNOWN, 4);
        drec.append_uint(VALUE_TCPBITS, 1);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP4  = "127.0.0.1";
    std::string VALUE_DST_IP4  = "8.8.8.8";
    uint16_t    VALUE_SRC_PORT = 65000;
    uint16_t    VALUE_DST_PORT = 80;
    uint8_t     VALUE_PROTO    = 6; // TCP
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670372999ULL;
    uint64_t    VALUE_BYTES    = 1234567;
    uint64_t    VALUE_PKTS     = 12345;
    double      VALUE_UNKNOWN  = 3.1416f;
    uint8_t     VALUE_TCPBITS  = 0x13; // ACK, SYN, FIN
};

// Convert Data Record with default flags to user provided buffer (without reallocation support)
TEST_F(Drec_basic, defaultConverter)
{
    size_t buffer_size = 2048;
    std::unique_ptr<char[]> buffer_data(new char[buffer_size]);

    const size_t buffer_size_orig = buffer_size;
    char *buffer_ptr = buffer_data.get();

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer_ptr, &buffer_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(strlen(buffer_ptr), size_t(rc));
    EXPECT_EQ(buffer_size, buffer_size_orig);

    // Try to parse the JSON string and check values
    Config cfg = parse_string(buffer_ptr, JSON, "drec2json");
    EXPECT_EQ((std::string) cfg["iana:sourceIPv4Address"], VALUE_SRC_IP4);
    EXPECT_EQ((std::string) cfg["iana:destinationIPv4Address"], VALUE_DST_IP4);
    EXPECT_EQ((uint16_t) cfg["iana:sourceTransportPort"], VALUE_SRC_PORT);
    EXPECT_EQ((uint16_t) cfg["iana:destinationTransportPort"], VALUE_DST_PORT);
    EXPECT_EQ((uint16_t) cfg["iana:protocolIdentifier"], VALUE_PROTO);
    EXPECT_EQ((uint64_t) cfg["iana:flowStartMilliseconds"], VALUE_TS_FST);
    EXPECT_EQ((uint64_t) cfg["iana:flowEndMilliseconds"], VALUE_TS_LST);
    EXPECT_EQ((uint64_t) cfg["iana:octetDeltaCount"], VALUE_BYTES);
    EXPECT_EQ((uint64_t) cfg["iana:packetDeltaCount"], VALUE_PKTS);
    EXPECT_EQ((uint8_t)  cfg["iana:tcpControlBits"], VALUE_TCPBITS);

    // Check if the field with unknown definition of IE is present
    EXPECT_TRUE(cfg.has_key("en10000:id100"));
    // Padding field(s) should not be in the JSON
    EXPECT_FALSE(cfg.has_key("iana:paddingOctets"));
}

// Convert Data Record to JSON and make the parser to allocate buffer for us
TEST_F(Drec_basic, defaultConverterWithAlloc)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    // Try to parse the JSON string and check values
    Config cfg;
    ASSERT_NO_THROW(cfg = parse_string(buffer, JSON, "drec2json"));

    free(buffer);
}

// Try to store the JSON to too short buffer
TEST_F(Drec_basic, tooShortBuffer)
{
    constexpr size_t BSIZE = 5U; // This should be always insufficient
    char buffer_data[BSIZE];
    size_t buffer_size = BSIZE;

    char *buffer_ptr = buffer_data;
    ASSERT_EQ(fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer_ptr, &buffer_size), FDS_ERR_BUFFER);
    EXPECT_EQ(buffer_size, BSIZE);
}

// Convert Data Record with realloca support
TEST_F(Drec_basic, allowRealloc)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    free(buff);
}

// Test for formating flag (FDS_CD2J_FORMAT_TCPFLAGS)
TEST_F(Drec_basic, tcpFlag)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_FORMAT_TCPFLAGS;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:tcpControlBits"], ".A..SF");

    free(buff);
}


// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record of a biflow
class Drec_biflow : public Drec_base { protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  7, 2);                    // sourceTransportPort
        trec.add_field( 27, 16);                   // sourceIPv6Address
        trec.add_field( 11, 2);                    // destinationTransportPort
        trec.add_field( 28, 16);                   // destinationIPv6Address
        trec.add_field(  4, 1);                    // protocolIdentifier
        trec.add_field(210, 3);                    // -- paddingOctets
        trec.add_field(156, 8);                    // flowStartNanoseconds
        trec.add_field(157, 8);                    // flowEndNanoseconds
        trec.add_field(156, 8, 29305);             // flowStartNanoseconds (reverse)
        trec.add_field(157, 8, 29305);             // flowEndNanoseconds   (reverse)
        trec.add_field( 96, ipfix_trec::SIZE_VAR); // applicationName
        trec.add_field( 94, ipfix_trec::SIZE_VAR); // applicationDescription
        trec.add_field(210, 5);                    // -- paddingOctets
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName (second occurrence)
        trec.add_field(  1, 8);                    // octetDeltaCount
        trec.add_field(  2, 4);                    // packetDeltaCount
        trec.add_field(  1, 8, 29305);             // octetDeltaCount (reverse)
        trec.add_field(  2, 4, 29305);             // packetDeltaCount (reverse)

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_ip(VALUE_SRC_IP6);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_ip(VALUE_DST_IP6);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_FST_R, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_datetime(VALUE_TS_LST_R, FDS_ET_DATE_TIME_NANOSECONDS);
        drec.append_string(VALUE_APP_NAME);      // Adds variable head automatically (short version)
        drec.var_header(VALUE_APP_DSC.length(), true); // Adds variable head manually (long version)
        drec.append_string(VALUE_APP_DSC, VALUE_APP_DSC.length());
        drec.append_uint(0, 5); // Padding
        drec.var_header(VALUE_IFC1.length(), false); // empty string (only header)
        drec.append_string(VALUE_IFC2);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 4);
        drec.append_uint(VALUE_BYTES_R, 8);
        drec.append_uint(VALUE_PKTS_R, 4);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP6  = "2001:db8::2:1";
    std::string VALUE_DST_IP6  = "fe80::fea9:6fc4:2e98:cdb2";
    uint16_t    VALUE_SRC_PORT = 1234;
    uint16_t    VALUE_DST_PORT = 8754;
    uint8_t     VALUE_PROTO    = 17; // UDP
    uint64_t    VALUE_TS_FST   = 1522670362000ULL;
    uint64_t    VALUE_TS_LST   = 1522670373000ULL;
    uint64_t    VALUE_TS_FST_R = 1522670364000ULL;
    uint64_t    VALUE_TS_LST_R = 1522670369000ULL;
    std::string VALUE_APP_NAME = "firefox";
    std::string VALUE_APP_DSC  = "linux/web browser";
    uint64_t    VALUE_BYTES    = 1234567;
    uint64_t    VALUE_PKTS     = 12345;
    uint64_t    VALUE_BYTES_R  = 7654321;
    uint64_t    VALUE_PKTS_R   = 54321;
    std::string VALUE_IFC1     = ""; // empty string
    std::string VALUE_IFC2     = "enp0s31f6";
};


// Convert Data Record with default flags to user provided buffer (without reallocation support)
TEST_F(Drec_biflow, simpleParser)
{
    // NOTE: "iana:interfaceName" has multiple occurrences, therefore, it MUST be converted
    //  into an array i.e. "iana:interfaceName" : ["", "enp0s31f6"]
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   EXPECT_TRUE(cfg["iana:interfaceName"].is_array());
   auto cfg_arr = cfg["iana:interfaceName"].as_array();
   EXPECT_EQ(cfg_arr.size(), 2U);
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC1), cfg_arr.end());
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC2), cfg_arr.end());
   free(buff);
}

// Convert Data Record with flag FDS_CD2J_NUMERIC_ID
TEST_F(Drec_biflow, numID)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_NUMERIC_ID;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg.has_key("en0:id7"));
    EXPECT_TRUE(cfg.has_key("en0:id27"));
    EXPECT_TRUE(cfg.has_key("en0:id11"));
    EXPECT_TRUE(cfg.has_key("en0:id28"));
    EXPECT_FALSE(cfg.has_key("en0:id210"));
    EXPECT_TRUE(cfg.has_key("en0:id156"));
    EXPECT_TRUE(cfg.has_key("en0:id157"));
    EXPECT_TRUE(cfg.has_key("en29305:id156"));
    EXPECT_TRUE(cfg.has_key("en29305:id157"));
    EXPECT_TRUE(cfg.has_key("en0:id96"));
    EXPECT_TRUE(cfg.has_key("en0:id94"));
    EXPECT_TRUE(cfg.has_key("en0:id82"));
    EXPECT_TRUE(cfg.has_key("en0:id1"));
    EXPECT_TRUE(cfg.has_key("en0:id2"));
    EXPECT_TRUE(cfg.has_key("en29305:id1"));
    EXPECT_TRUE(cfg.has_key("en29305:id2"));

    EXPECT_EQ((uint64_t) cfg["en0:id1"], VALUE_BYTES);     // octetDeltaCount
    EXPECT_EQ((uint64_t) cfg["en0:id2"], VALUE_PKTS);      // packetDeltaCount
    EXPECT_EQ((uint64_t) cfg["en0:id7"], VALUE_SRC_PORT);  // sourceTransportPort
    EXPECT_EQ( cfg["en0:id27"], VALUE_SRC_IP6);            // sourceIPv6Address
    EXPECT_EQ((uint64_t) cfg["en0:id11"], VALUE_DST_PORT); // destinationTransportPort
    EXPECT_EQ( cfg["en0:id28"], VALUE_DST_IP6);            // destinationIPv6Address
    EXPECT_EQ((uint64_t) cfg["en0:id4"], VALUE_PROTO);     // protocolIdentifier
    EXPECT_EQ((uint64_t) cfg["en0:id156"], VALUE_TS_FST);  // flowStartNanoseconds
    EXPECT_EQ((uint64_t) cfg["en0:id157"], VALUE_TS_LST);  // flowEndNanoseconds
    EXPECT_EQ(cfg["en0:id96"], VALUE_APP_NAME);            // applicationName
    EXPECT_EQ(cfg["en0:id94"], VALUE_APP_DSC);             // applicationDescription
    free(buff);
}

// Convert Data Record from reverse point of view
TEST_F(Drec_biflow, reverseView)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_NUMERIC_ID | FDS_CD2J_BIFLOW_REVERSE;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");

    EXPECT_EQ((uint64_t) cfg["en0:id1"], VALUE_BYTES_R);     // octetDeltaCount (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id2"], VALUE_PKTS_R);      // packetDeltaCount (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id156"], VALUE_TS_FST_R);  // flowStartNanoseconds (reverse)
    EXPECT_EQ((uint64_t) cfg["en0:id157"], VALUE_TS_LST_R);  // flowEndNanoseconds   (reverse)
    free(buff);
}
// Testing return of error code FDS_ERR_BUFFER
TEST_F(Drec_biflow, errorBuff)
{
    // Default situation
    constexpr size_t BSIZE = 0U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);
    free(def_buff);
    // Loop check error situations
    for (int i = 0; i < def_rc; i++){
        char*  new_buff= (char*) malloc(i);
        uint32_t new_flags = 0;
        size_t new_buff_size = i;
        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_EQ(new_rc, FDS_ERR_BUFFER);
        free(new_buff);
    }
}

// Test for time format (FDS_CD2J_TS_FORMAT_MSEC)
TEST_F(Drec_biflow, timeFormat)
{
    constexpr size_t BSIZE = 0U;
    size_t buff_size = BSIZE;
    char* buff = NULL;
    uint32_t flags = FDS_CD2J_TS_FORMAT_MSEC;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ( cfg["iana:flowStartNanoseconds"], "2018-04-02T11:59:22.000Z");
    EXPECT_EQ( cfg["iana:flowEndNanoseconds"], "2018-04-02T11:59:33.000Z");

    free (buff);
}

// Test for string format of protocol (FDS_CD2J_FORMAT_PROTO)
TEST_F(Drec_biflow, protoFormat)
{
    constexpr size_t BSIZE = 2000U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_FORMAT_PROTO;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ( cfg["iana:protocolIdentifier"], "UDP");

    free(buff);
}

// Test fro nonprintable characters (FDS_CD2J_NON_PRINTABLE)
TEST_F(Drec_biflow, nonPrint)
{
    constexpr size_t BSIZE = 2000U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_NON_PRINTABLE;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    Config cfg = parse_string(buff, JSON, "drec2json");

    free(buff);
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record for extra situations
class Drec_extra : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  8, 4);             // sourceIPv4Address
        trec.add_field( 12, 4);             // destinationIPv4Address
        trec.add_field( 94, ipfix_trec::SIZE_VAR);// applicationDescription (string)
        trec.add_field(  7, 2);             // sourceTransportPort
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field(  4, 1);             // protocolIdentifier
        trec.add_field(210, 3);             // -- paddingOctets
        trec.add_field(152, 8);             // flowStartMilliseconds
        trec.add_field(153, 8);             // flowEndMilliseconds
        trec.add_field(  1, 8);             // octetDeltaCount
        trec.add_field(  2, 8);             // packetDeltaCount
        trec.add_field(100, 8, 10000);      // -- field with unknown definition --
        trec.add_field(  6, 2);             // tcpControlBits
        trec.add_field(1001,1);             // myBool
        trec.add_field(1000,8);             // myFloat64
        trec.add_field(1003,4);             // myFloat32
        trec.add_field(1002,8);             // myInt
        trec.add_field(1004,8);             // myPInf
        trec.add_field(1005,8);             // myMInf
        trec.add_field(1006,8);             // myNan
        trec.add_field(83,ipfix_trec::SIZE_VAR); //interfaceDescription
        trec.add_field(56, 6);              // sourceMacAddress
        trec.add_field(95,10);              // applicationId

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_string(VALUE_APP_DES);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.append_uint(0, 3); // Padding
        drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_datetime(VALUE_TS_LST, FDS_ET_DATE_TIME_MILLISECONDS);
        drec.append_uint(VALUE_BYTES, 8);
        drec.append_uint(VALUE_PKTS, 8);
        drec.append_float(VALUE_UNKNOWN, 8);
        drec.append_uint(VALUE_TCPBITS, 2);
        drec.append_bool(VALUE_MY_BOOL);
        drec.append_float(VALUE_MY_FLOAT64, 8);
        drec.append_float(VALUE_MY_FLOAT32, 4);
        drec.append_int(VALUE_MY_INT,8);
        drec.append_float(VALUE_MY_PINF,8);
        drec.append_float(VALUE_MY_MINF,8);
        drec.append_float(VALUE_MY_NAN, 8);
        drec.append_string(VALUE_INF_DES);
        drec.append_mac(VALUE_SRC_MAC);
        drec.append_octets(VALUE_APP_ID.c_str(),(uint16_t)10, false);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_SRC_IP4    = "127.0.0.1";
    std::string VALUE_DST_IP4    = "8.8.8.8";
    std::string VALUE_APP_DES    = "web\\\nclose\t\"open\bdog\fcat\r\"\x13";
    std::string VALUE_INF_DES    = "prety=white+ cleannothing$";
    double      VALUE_MY_PINF    = std::numeric_limits<double>::infinity();
    double      VALUE_MY_MINF    = -std::numeric_limits<double>::infinity();
    double      VALUE_MY_NAN     = NAN;
    uint16_t    VALUE_SRC_PORT   = 65000;
    uint16_t    VALUE_DST_PORT   = 80;
    uint8_t     VALUE_PROTO      = 6; // TCP
    uint64_t    VALUE_TS_FST     = 1522670362000ULL;
    uint64_t    VALUE_TS_LST     = 1522670372999ULL;
    uint64_t    VALUE_BYTES      = 1234567;
    uint64_t    VALUE_PKTS       = 12345;
    double      VALUE_UNKNOWN    = 3.141233454443216f;
    uint8_t     VALUE_TCPBITS    = 0x13; // ACK, SYN, FIN
    bool        VALUE_MY_BOOL    = true;
    double      VALUE_MY_FLOAT64 = 0.1234;
    double      VALUE_MY_FLOAT32 = 0.5678;
    signed      VALUE_MY_INT     = 1006;
    std::string VALUE_SRC_MAC    = "01:12:1F:13:11:8A";
    std::string VALUE_APP_ID     = "\x33\x23\x24\x30\x31\x32\x34\x35\x36\x37"; // 3#$0124567

};

// Test for diferent data types
TEST_F(Drec_extra, testTypes)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((double)cfg["iana:myFloat64"], VALUE_MY_FLOAT64);
    EXPECT_EQ((double)cfg["iana:myFloat32"], VALUE_MY_FLOAT32);
    EXPECT_EQ(cfg["iana:myBool"], VALUE_MY_BOOL);
    EXPECT_EQ((signed)cfg["iana:myInt"], VALUE_MY_INT);
    EXPECT_EQ(cfg["iana:sourceMacAddress"], VALUE_SRC_MAC);

    free(buff);
}

// Test for non printable characters (FDS_CD2J_NON_PRINTABLE)
TEST_F(Drec_extra, nonPrintable)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_NON_PRINTABLE | FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ(cfg["iana:applicationDescription"], "web\\close\"opendogcat\"");

    free(buff);
}

// Test for eskaping characters
TEST_F(Drec_extra, printableChar)
{
    constexpr size_t BSIZE = 10U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");
    // For conversion from JSON to C natation cares JSON parser
    EXPECT_EQ((std::string)cfg["iana:applicationDescription"], VALUE_APP_DES);

    free(buff);
}

// Test for NAN, +INF, -INF values
TEST_F(Drec_extra, extraValue)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_TRUE(cfg["iana:myNan"].is_string());
    EXPECT_EQ(cfg["iana:myNan"],"NaN");
    EXPECT_TRUE(cfg["iana:myPInf"].is_string());
    EXPECT_EQ(cfg["iana:myPInf"],"Infinity");
    EXPECT_TRUE(cfg["iana:myMInf"].is_string());
    EXPECT_EQ(cfg["iana:myMInf"],"-Infinity");

    free(buff);
}

// Test for other ASCII characters
TEST_F(Drec_extra, otherChar)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ(cfg["iana:interfaceDescription"], VALUE_INF_DES);

    free(buff);
}

// Test for other MAC adress
TEST_F(Drec_extra, macAdr)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ(cfg["iana:sourceMacAddress"], VALUE_SRC_MAC);

    free(buff);
}

// Test for octet values
TEST_F(Drec_extra, octVal)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:applicationId"], "0x33232430313234353637");

    free(buff);
}

// Test for executing branches with memory realocation
TEST_F(Drec_extra, forLoop)
{
    constexpr size_t BSIZE = 1U;
    char* def_buff = (char*) malloc(BSIZE);
    uint32_t def_flags = FDS_CD2J_ALLOW_REALLOC;
    size_t def_buff_size = BSIZE;

    int def_rc = fds_drec2json(&m_drec, def_flags, m_iemgr.get(), &def_buff, &def_buff_size);
    ASSERT_GT(def_rc, 0);
    EXPECT_NE(def_buff_size, BSIZE);
    EXPECT_EQ(size_t(def_rc), strlen(def_buff));
    EXPECT_NE(def_buff_size, BSIZE);

    free(def_buff);
    // Loop check right situations
    for (int i = 0; i < def_rc; i++){
        char*  new_buff = (char*) malloc(i);
        uint32_t new_flags = FDS_CD2J_ALLOW_REALLOC;
        size_t new_buff_size = i;

        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        EXPECT_GT(new_rc, 0);
        free(new_buff);
    }

}

// Test for formating flag of size 2 (FDS_CD2J_FORMAT_TCPFLAGS)
TEST_F(Drec_extra, flagSize2)
{
    constexpr size_t BSIZE = 5U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC | FDS_CD2J_FORMAT_TCPFLAGS;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    EXPECT_NE(buff_size, BSIZE);
    Config cfg = parse_string(buff, JSON, "drec2json");
    EXPECT_EQ((std::string)cfg["iana:tcpControlBits"], ".A..SF");

    free(buff);
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record for unvalid situations
class Drec_unvalid : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field( 8,  0);             // sourceIPv4Address
        trec.add_field(12,  0);             // destinationIPv4Address (second occurrence)
        trec.add_field(24,  0);             // postPacketDeltaCount
        trec.add_field(1002,0);             // myInt
        trec.add_field(1003,0);             // myFloat32
        trec.add_field(1000,0);             // myFloat64
        trec.add_field(156, 0);             // flowStartNanoseconds
        trec.add_field(  4, 0);             // protocolIdentifier
        trec.add_field(  6, 0);             // tcpControlBits
        trec.add_field(56,  0);             // sourceMacAddress
        trec.add_field( 12, 4);             // destinationIPv4Address
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field( 82, ipfix_trec::SIZE_VAR); // interfaceName
        trec.add_field( 82, 0);             // interfaceName (second occurrence)
        trec.add_field(1001,2);             // myBool

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_string(VALUE_IFC1); // empty string (only header)
        drec.append_string(VALUE_IFC2);
        drec.append_bool(VALUE_MY_BOOL);

        register_template(trec);
        drec_create(256, drec);
    }

    std::string VALUE_DST_IP4    = "8.8.8.8";
    std::string VALUE_IFC1       = "qwert";           // empty string
    std::string VALUE_IFC2       = "enp0s31f6";
    uint16_t    VALUE_SRC_PORT   = 65000;
    uint16_t    VALUE_DST_PORT   = 80;
    bool        VALUE_MY_BOOL    = true;

    // drec.append_datetime(VALUE_TS_FST, FDS_ET_DATE_TIME_NANOSECONDS);

    // uint64_t    VALUE_TS_FST   = 1522670362000ULL;

};

//  Test for adding null in case of unvalid field
TEST_F(Drec_unvalid, unvalidField)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   EXPECT_TRUE(cfg["iana:sourceIPv4Address"].is_null());
   EXPECT_TRUE(cfg["iana:myBool"].is_null());
   EXPECT_TRUE(cfg["iana:postPacketDeltaCount"].is_null());
   EXPECT_TRUE(cfg["iana:myInt"].is_null());
   EXPECT_TRUE(cfg["iana:myFloat32"].is_null());
   EXPECT_TRUE(cfg["iana:myFloat64"].is_null());
   EXPECT_TRUE(cfg["iana:flowStartNanoseconds"].is_null());
   EXPECT_TRUE(cfg["iana:protocolIdentifier"].is_null());
   EXPECT_TRUE(cfg["iana:tcpControlBits"].is_null());
   EXPECT_TRUE(cfg["iana:sourceMacAddress"].is_null());

   free(buff);
}

//  Test for adding null to multifield in case of unvalid field
TEST_F(Drec_unvalid, nullInMulti)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

    int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
    ASSERT_GT(rc, 0);
    EXPECT_EQ(size_t(rc), strlen(buff));
    Config cfg = parse_string(buff, JSON, "drec2json");

    ASSERT_TRUE(cfg["iana:destinationIPv4Address"].is_array());
    auto cfg_arr = cfg["iana:interfaceName"].as_array();
    EXPECT_EQ(cfg_arr.size(), 2U);
    EXPECT_EQ(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_DST_IP4), cfg_arr.end());
    EXPECT_EQ(std::find(cfg_arr.begin(), cfg_arr.end(), NULL), cfg_arr.end());

    free(buff);
}

// Test fot string with size 0
TEST_F(Drec_unvalid, zeroSizeStr)
{
    constexpr size_t BSIZE = 2U;
    char* buff = (char*) malloc(BSIZE);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;
    size_t buff_size = BSIZE;

   int rc = fds_drec2json(&m_drec, flags, m_iemgr.get(), &buff, &buff_size);
   ASSERT_GT(rc, 0);
   EXPECT_EQ(size_t(rc), strlen(buff));
   Config cfg = parse_string(buff, JSON, "drec2json");
   ASSERT_TRUE(cfg["iana:interfaceName"].is_array());
   auto cfg_arr = cfg["iana:interfaceName"].as_array();
   EXPECT_EQ(cfg_arr.size(), 2U);
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), VALUE_IFC1), cfg_arr.end());
   EXPECT_NE(std::find(cfg_arr.begin(), cfg_arr.end(), ""), cfg_arr.end());

   free(buff);
}

// -------------------------------------------------------------------------------------------------
/// IPFIX Data Record with basicList
class Drec_basicLists : public Drec_base {
protected:
    /// Before each Test case
    void SetUp() override {
        Drec_base::SetUp();

        // Prepare an IPFIX Template
        ipfix_trec trec{256};
        trec.add_field(  8, 4);             // sourceIPv4Address
        trec.add_field( 12, 4);             // destinationIPv4Address
        trec.add_field(  7, 2);             // sourceTransportPort
        trec.add_field( 11, 2);             // destinationTransportPort
        trec.add_field(  4, 1);             // protocolIdentifier
        trec.add_field(484, ipfix_trec::SIZE_VAR); // bgpSourceCommunityList (empty)
        trec.add_field(485, ipfix_trec::SIZE_VAR); // bgpDestinationCommunityList (non-empty)
        trec.add_field(291, ipfix_trec::SIZE_VAR); // basicList (of observationDomainName strings)

        // Prepare an empty basicList (i.e. bgpSourceCommunityList of bgpCommunity)
        ipfix_blist blist_empty;
        blist_empty.header_short(FDS_IPFIX_LIST_NONE_OF, 483, 4);

        // Prepare a single element basicList (i.e. bgpDestinationCommunityList of bgpCommunity)
        ipfix_field fields_one;
        fields_one.append_uint(VALUE_BGP_DST, 4);
        ipfix_blist blist_one;
        blist_one.header_short(FDS_IPFIX_LIST_ALL_OF, 483, 4);
        blist_one.append_field(fields_one);

        // Prepare a basicList of strings (i.e. basicList of observationDomainName)
        ipfix_field fields_multi;
        fields_multi.append_string(VALUE_BLIST_STR1);
        fields_multi.var_header(VALUE_BLIST_STR2.length(), false); // empty string (only header)
        fields_multi.append_string(VALUE_BLIST_STR3);
        ipfix_blist blist_multi;
        blist_multi.header_short(FDS_IPFIX_LIST_UNDEFINED, 300, FDS_IPFIX_VAR_IE_LEN);
        blist_multi.append_field(fields_multi);

        // Prepare an IPFIX Data Record
        ipfix_drec drec{};
        drec.append_ip(VALUE_SRC_IP4);
        drec.append_ip(VALUE_DST_IP4);
        drec.append_uint(VALUE_SRC_PORT, 2);
        drec.append_uint(VALUE_DST_PORT, 2);
        drec.append_uint(VALUE_PROTO, 1);
        drec.var_header(blist_empty.size()); // bgpSourceCommunityList
        drec.append_blist(blist_empty);
        drec.var_header(blist_one.size());   // bgpDestinationCommunityList
        drec.append_blist(blist_one);
        drec.var_header(blist_multi.size()); // basicList
        drec.append_blist(blist_multi);

        register_template(trec);
        drec_create(256, drec);
    }

    uint32_t    VALUE_BGP_DST    =    23;
    std::string VALUE_BLIST_STR1 = "RandomString";
    std::string VALUE_BLIST_STR2 = "";
    std::string VALUE_BLIST_STR3 = "Another non-empty string";
    std::string VALUE_SRC_IP4    = "127.0.0.1";
    std::string VALUE_DST_IP4    = "8.8.8.8";
    uint16_t    VALUE_SRC_PORT   = 65000;
    uint16_t    VALUE_DST_PORT   = 80;
    uint8_t     VALUE_PROTO      = 6; // TCP
};

TEST_F(Drec_basicLists, simple)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;

    int rc = fds_drec2json(&m_drec, 0, m_iemgr.get(), &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    free(buffer);
}

TEST_F(Drec_basicLists, rightValues)
{
    char *buffer = nullptr;
    size_t buffer_size = 0;
    uint32_t flags = 0;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));

    Config cfg = parse_string(buffer, JSON, "drec2json");
    ASSERT_TRUE(cfg["iana:bgpSourceCommunityList"].is_object());
    ASSERT_TRUE(cfg["iana:bgpDestinationCommunityList"].is_object());
    ASSERT_TRUE(cfg["iana:basicList"].is_object());

    auto& src_obj = cfg["iana:bgpSourceCommunityList"];
    auto& dst_obj = cfg["iana:bgpDestinationCommunityList"];
    auto& basic_obj = cfg["iana:basicList"];


    EXPECT_EQ(src_obj["semantic"], "noneOf");
    EXPECT_EQ(dst_obj["semantic"], "allOf");
    EXPECT_EQ(basic_obj["semantic"], "undefined");

    EXPECT_TRUE(src_obj["data"].is_array());
    EXPECT_TRUE(dst_obj["data"].is_array());
    EXPECT_TRUE(basic_obj["data"].is_array());

    auto dst_data_arr = dst_obj["data"].as_array();
    auto basic_data_arr = basic_obj["data"].as_array();

    EXPECT_NE(std::find(dst_data_arr.begin(), dst_data_arr.end(), VALUE_BGP_DST), dst_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR1), basic_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR2), basic_data_arr.end());
    EXPECT_NE(std::find(basic_data_arr.begin(), basic_data_arr.end(), VALUE_BLIST_STR3), basic_data_arr.end());

    free(buffer);
}

TEST_F(Drec_basicLists, allocLoop)
{
    size_t buffer_size = 2U;
    char* buffer = (char*) malloc(buffer_size);
    uint32_t flags = FDS_CD2J_ALLOW_REALLOC;

    auto iemgr = m_iemgr.get();
    int rc = fds_drec2json(&m_drec, flags, iemgr, &buffer, &buffer_size);
    ASSERT_GT(rc, 0);
    ASSERT_NE(buffer, nullptr);
    EXPECT_NE(buffer_size, 0U);
    EXPECT_EQ(strlen(buffer), size_t(rc));
    Config cfg = parse_string(buffer, JSON, "drec2json");


    for (int i = 0; i < rc; i++){
        size_t new_buff_size = i;
        char*  new_buff= (char *) malloc(new_buff_size);
        uint32_t new_flags = FDS_CD2J_ALLOW_REALLOC;

        int new_rc = fds_drec2json(&m_drec, new_flags, m_iemgr.get(), &new_buff, &new_buff_size);
        ASSERT_GT(new_rc, 0);

        free(new_buff);
    }

    free(buffer);

}

/* TODO

*/
