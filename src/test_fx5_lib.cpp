#include <QtTest/QtTest>

extern "C" {
#include "melsec_pdu_request.h"

}

class TestPdu : public QObject
{
    Q_OBJECT

private slots:
    void read_d_words_single();
    void read_d_words_errors();
    void write_d_words_single();
    void write_d_words_multi();
    void write_d_words_errors();
    void roundtrip_read_d();
    void roundtrip_write_d_single();
    void roundtrip_write_d_multi();
    void parse_unsupported_modified_cmd();

    void read_x_bits_1bit();
    void read_x_bits_16bits();
    void read_x_bits_17bits();
    void read_m_bits_17bits();

    void write_y_bits_1bit();
    void write_y_bits_17bits_pattern();

    void roundtrip_read_x_bits_words_count();
    void roundtrip_read_m_bits_words_count();
    void roundtrip_write_y_bits_words_and_data();
    void roundtrip_write_m_bits_words_and_data();


};


static uint16_t bitsToWords(uint16_t bitCount)
{
    return (uint16_t)((bitCount + 15u) / 16u);
}

// pack bits01 (0/1) into little-endian words stored in bytes (len = words*2)
static QByteArray packBitsToWordsLE(const uint8_t* bits01, uint16_t bitCount)
{
    const uint16_t words = bitsToWords(bitCount);
    QByteArray out;
    out.resize(words * 2);
    out.fill(char(0));

    for (uint16_t i = 0; i < bitCount; ++i) {
        if (bits01[i]) {
            const uint16_t w = (uint16_t)(i / 16u);
            const uint16_t b = (uint16_t)(i % 16u);
            const int off = int(w * 2u);

            uint16_t word = (uint8_t)out[off] | ((uint16_t)(uint8_t)out[off + 1] << 8);
            word = (uint16_t)(word | (uint16_t)(1u << b));
            out[off]     = char(word & 0xFFu);
            out[off + 1] = char((word >> 8) & 0xFFu);
        }
    }
    return out;
}


static void expectBytes(const uint8_t* actual, int actualLen,
                        const uint8_t* expected, int expectedLen)
{
    QCOMPARE(actualLen, expectedLen);
    for (int i = 0; i < expectedLen; ++i) {
        QCOMPARE(actual[i], expected[i]);
    }
}

void TestPdu::read_d_words_single()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // Read D200 count=1
    const auto st = pdu_build_read_d_words_request(200u, 1u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x04,       // cmd 0x0401 LE
        0x00, 0x00,       // sub 0x0000 LE
        0xC8, 0x00, 0x00, // address 200 (u24 LE)
        0xA8,             // dev D
        0x01, 0x00        // count 1 (u16 LE)
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::read_d_words_errors()
{
    uint8_t buf[16] = {0};
    uint16_t outSize = 0;

    // null buffer
    QCOMPARE(pdu_build_read_d_words_request(0u, 1u, nullptr, 16, &outSize), PDU_ERR_NULL);

    // null size
    QCOMPARE(pdu_build_read_d_words_request(0u, 1u, buf, 16, nullptr), PDU_ERR_NULL);

    // count=0
    QCOMPARE(pdu_build_read_d_words_request(0u, 0u, buf, 16, &outSize), PDU_ERR_COUNT_ZERO);

    // address out of 24-bit
    QCOMPARE(pdu_build_read_d_words_request(0x01000000u, 1u, buf, 16, &outSize), PDU_ERR_ADDR_RANGE);

    // too small
    QCOMPARE(pdu_build_read_d_words_request(0u, 1u, buf, 9, &outSize), PDU_ERR_TOO_SMALL);
}

void TestPdu::write_d_words_single()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint16_t v = 0x04D2; // 1234 dec
    const auto st = pdu_build_write_d_words_request(200u, &v, 1u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x14,       // cmd 0x1401 LE
        0x00, 0x00,       // sub 0x0000 LE
        0xC8, 0x00, 0x00, // address 200 (u24 LE)
        0xA8,             // dev D
        0x01, 0x00,       // count 1
        0xD2, 0x04        // value 0x04D2 LE
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::write_d_words_multi()
{
    uint8_t buf[64] = {0};
    uint16_t outSize = 0;

    const uint16_t values[3] = { 0x0001, 0x0203, 0xBEEF };
    const auto st = pdu_build_write_d_words_request(0u, values, 3u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x14,
        0x00, 0x00,
        0x00, 0x00, 0x00,
        0xA8,
        0x03, 0x00,
        0x01, 0x00,   // 0x0001
        0x03, 0x02,   // 0x0203
        0xEF, 0xBE    // 0xBEEF
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::write_d_words_errors()
{
    uint8_t buf[16] = {0};
    uint16_t outSize = 0;
    const uint16_t v = 1;

    // values null
    QCOMPARE(pdu_build_write_d_words_request(0u, nullptr, 1u, buf, sizeof(buf), &outSize), PDU_ERR_VALUES_NULL);

    // count=0
    QCOMPARE(pdu_build_write_d_words_request(0u, &v, 0u, buf, sizeof(buf), &outSize), PDU_ERR_COUNT_ZERO);

    // address out of range
    QCOMPARE(pdu_build_write_d_words_request(0x01000000u, &v, 1u, buf, sizeof(buf), &outSize), PDU_ERR_ADDR_RANGE);

    // too small: need 10 + 2*1 = 12, give 11
    QCOMPARE(pdu_build_write_d_words_request(0u, &v, 1u, buf, 11, &outSize), PDU_ERR_TOO_SMALL);
}

void TestPdu::roundtrip_read_d()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint32_t inAddr = 12345u;
    const uint16_t inCount = 7u;

    QCOMPARE(pdu_build_read_d_words_request(inAddr, inCount, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_READ_D_REGISTERS);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, inCount);
}

void TestPdu::roundtrip_write_d_single()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint32_t inAddr = 200u;
    const uint16_t inVal  = 0x04D2;

    QCOMPARE(pdu_build_write_d_words_request(inAddr, &inVal, 1u, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_WRITE_D_REGISTERS);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, (uint16_t)1u);

    QCOMPARE(cmd.values.count, (uint16_t)1u);
    QCOMPARE(pdu_word_view_get(&cmd.values, 0u), inVal);
}

void TestPdu::roundtrip_write_d_multi()
{
    uint8_t buf[64] = {0};
    uint16_t outSize = 0;

    const uint32_t inAddr = 0u;
    const uint16_t values[3] = { 0x0001, 0x0203, 0xBEEF };

    QCOMPARE(pdu_build_write_d_words_request(inAddr, values, 3u, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_WRITE_D_REGISTERS);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, (uint16_t)3u);

    QCOMPARE(cmd.values.count, (uint16_t)3u);
    for (uint16_t i = 0; i < 3u; ++i) {
        QCOMPARE(pdu_word_view_get(&cmd.values, i), values[i]);
    }
}

void TestPdu::parse_unsupported_modified_cmd()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // Build a valid read D PDU...
    QCOMPARE(pdu_build_read_d_words_request(10u, 1u, buf, sizeof(buf), &outSize), PDU_OK);

    // ...then corrupt command low byte (0x0401 -> 0x0402 for example)
    buf[0] ^= 0x03;

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_ERR_UNSUPPORTED);
}

void TestPdu::read_x_bits_1bit()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // X address=0x012345, count_bits=1 => words=1
    const auto st = pdu_build_read_x_bits_request(0x012345u, 1u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x04,             // cmd 0x0401
        0x00, 0x00,             // sub word
        0x45, 0x23, 0x01,       // head device no (u24 LE)
        PDU_DEV_X,              // X
        0x01, 0x00              // points (words)=1
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::read_x_bits_16bits()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // count_bits=16 => words=1
    const auto st = pdu_build_read_x_bits_request(0u, 16u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x04,
        0x00, 0x00,
        0x00, 0x00, 0x00,
        PDU_DEV_X,
        0x01, 0x00
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::read_x_bits_17bits()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // count_bits=17 => words=2
    const auto st = pdu_build_read_x_bits_request(0u, 17u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x04,
        0x00, 0x00,
        0x00, 0x00, 0x00,
        PDU_DEV_X,
        0x02, 0x00
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::read_m_bits_17bits()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    // M address=100, count_bits=17 => words=2
    const auto st = pdu_build_read_m_bits_request(100u, 17u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x04,
        0x00, 0x00,
        0x64, 0x00, 0x00,   // 100
        PDU_DEV_M,
        0x02, 0x00
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::write_y_bits_1bit()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint8_t bits01[1] = { 1 }; // write Y0 = ON
    // bit_count=1 => words=1 => data 2 bytes (word0 bit0 set => 0x0001 => 01 00)

    const auto st = pdu_build_write_y_bits_request(0u, bits01, 1u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x14,         // cmd 0x1401
        0x00, 0x00,         // sub word
        0x00, 0x00, 0x00,   // head
        PDU_DEV_Y,
        0x01, 0x00,         // points (words)=1
        0x01, 0x00          // data word0 = 0x0001
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}

void TestPdu::write_y_bits_17bits_pattern()
{
    uint8_t buf[64] = {0};
    uint16_t outSize = 0;

    // pattern: set bit0=1, bit1=0, bit2=1, ... alternating for 17 bits
    uint8_t bits01[17];
    for (int i = 0; i < 17; ++i) {
        bits01[i] = (uint8_t)((i % 2) == 0 ? 1 : 0);
    }

    // 17 bits => words=2
    // word0 bits0..15 = 0b0101... (LSB bit0=1) => 0x5555 -> bytes 55 55
    // word1 bit16 corresponds to word1 bit0 => 1 -> 0x0001 -> bytes 01 00
    const auto st = pdu_build_write_y_bits_request(0u, bits01, 17u, buf, sizeof(buf), &outSize);
    QCOMPARE(st, PDU_OK);

    const uint8_t exp[] = {
        0x01, 0x14,
        0x00, 0x00,
        0x00, 0x00, 0x00,
        PDU_DEV_Y,
        0x02, 0x00,     // points=2 words
        0x55, 0x55,     // word0 = 0x5555
        0x01, 0x00      // word1 = 0x0001
    };

    expectBytes(buf, outSize, exp, (int)sizeof(exp));
}


void TestPdu::roundtrip_read_x_bits_words_count()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint32_t inAddr = 0x012345u;
    const uint16_t inBits = 17u;
    const uint16_t expWords = bitsToWords(inBits);

    QCOMPARE(pdu_build_read_x_bits_request(inAddr, inBits, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_READ_COIL_X);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, expWords);   // parser words-ot ad vissza (jelenlegi design)
}

void TestPdu::roundtrip_read_m_bits_words_count()
{
    uint8_t buf[32] = {0};
    uint16_t outSize = 0;

    const uint32_t inAddr = 100u;
    const uint16_t inBits = 1u;
    const uint16_t expWords = bitsToWords(inBits);

    QCOMPARE(pdu_build_read_m_bits_request(inAddr, inBits, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_READ_M_COIL);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, expWords);
}

void TestPdu::roundtrip_write_y_bits_words_and_data()
{
    uint8_t buf[64] = {0};
    uint16_t outSize = 0;

    // 17 bits alternating: 1,0,1,0,...
    uint8_t bits01[17];
    for (int i = 0; i < 17; ++i) bits01[i] = (uint8_t)(((i % 2) == 0) ? 1 : 0);

    const uint32_t inAddr = 0u;
    const uint16_t inBits = 17u;
    const uint16_t expWords = bitsToWords(inBits);

    QCOMPARE(pdu_build_write_y_bits_request(inAddr, bits01, inBits, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_WRITE_COIL_Y);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, expWords);

    // values: words*2 bytes
    QCOMPARE(cmd.values.count, expWords);

    const QByteArray expPacked = packBitsToWordsLE(bits01, inBits);
    QCOMPARE(expPacked.size(), int(expWords * 2u));

    // compare packed payload bytes
    for (int i = 0; i < expPacked.size(); ++i) {
        QCOMPARE(cmd.values.bytes[i], (uint8_t)expPacked[i]);
    }
}

void TestPdu::roundtrip_write_m_bits_words_and_data()
{
    uint8_t buf[64] = {0};
    uint16_t outSize = 0;

    // 1 bit set
    const uint8_t bits01[1] = { 1 };

    const uint32_t inAddr = 123u;
    const uint16_t inBits = 1u;
    const uint16_t expWords = bitsToWords(inBits);

    QCOMPARE(pdu_build_write_m_bits_request(inAddr, bits01, inBits, buf, sizeof(buf), &outSize), PDU_OK);

    pdu_command_t cmd{};
    QCOMPARE(pdu_parse_request(buf, outSize, &cmd), PDU_OK);

    QCOMPARE(cmd.cmd, PDU_WRITE_M_COIL);
    QCOMPARE(cmd.address, inAddr);
    QCOMPARE(cmd.count, expWords);

    QCOMPARE(cmd.values.count, expWords);

    const QByteArray expPacked = packBitsToWordsLE(bits01, inBits);
    QCOMPARE(expPacked.size(), int(expWords * 2u));
    for (int i = 0; i < expPacked.size(); ++i) {
        QCOMPARE(cmd.values.bytes[i], (uint8_t)expPacked[i]);
    }
}




QTEST_MAIN(TestPdu)
#include "test_pdu.moc"
