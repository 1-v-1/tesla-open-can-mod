#include <unity.h>
#include "can_frame_types.h"
#include "can_helpers.h"

void setUp() {}
void tearDown() {}

// --- setBit ---

void test_setBit_sets_bit0_of_byte0()
{
    CanFrame f = {};
    setBit(f, 0, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, f.data[0]);
}

void test_setBit_sets_bit7_of_byte0()
{
    CanFrame f = {};
    setBit(f, 7, true);
    TEST_ASSERT_EQUAL_HEX8(0x80, f.data[0]);
}

void test_setBit_sets_bit_in_byte5()
{
    CanFrame f = {};
    setBit(f, 46, true); // byte 5, bit 6
    TEST_ASSERT_EQUAL_HEX8(0x40, f.data[5]);
}

void test_setBit_sets_bit_in_byte7()
{
    CanFrame f = {};
    setBit(f, 60, true); // byte 7, bit 4
    TEST_ASSERT_EQUAL_HEX8(0x10, f.data[7]);
}

void test_setBit_clears_bit()
{
    CanFrame f = {};
    f.data[2] = 0xFF;
    setBit(f, 19, false); // byte 2, bit 3
    TEST_ASSERT_EQUAL_HEX8(0xF7, f.data[2]);
}

void test_setBit_does_not_affect_other_bytes()
{
    CanFrame f = {};
    f.data[0] = 0xAA;
    f.data[1] = 0xBB;
    setBit(f, 8, true); // byte 1, bit 0
    TEST_ASSERT_EQUAL_HEX8(0xAA, f.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, f.data[1]);
}

// --- readMuxID ---

void test_readMuxID_extracts_lower_3_bits()
{
    CanFrame f = {};
    f.data[0] = 0x05;
    TEST_ASSERT_EQUAL_UINT8(5, readMuxID(f));
}

void test_readMuxID_masks_upper_bits()
{
    CanFrame f = {};
    f.data[0] = 0xFA; // binary: 11111010 -> lower 3 = 010 = 2
    TEST_ASSERT_EQUAL_UINT8(2, readMuxID(f));
}

void test_readMuxID_zero()
{
    CanFrame f = {};
    f.data[0] = 0x00;
    TEST_ASSERT_EQUAL_UINT8(0, readMuxID(f));
}

void test_readMuxID_max_value()
{
    CanFrame f = {};
    f.data[0] = 0x07;
    TEST_ASSERT_EQUAL_UINT8(7, readMuxID(f));
}

// --- isFSDSelectedInUI ---

void test_isFSDSelectedInUI_true_when_bit6_set()
{
    CanFrame f = {};
    f.data[4] = 0x40; // bit 6 set
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_false_when_bit6_clear()
{
    CanFrame f = {};
    f.data[4] = 0x00;
    TEST_ASSERT_FALSE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_ignores_other_bits()
{
    CanFrame f = {};
    f.data[4] = 0xBF; // all bits set except bit 6
    TEST_ASSERT_FALSE(isFSDSelectedInUI(f));
}

void test_isFSDSelectedInUI_true_with_other_bits()
{
    CanFrame f = {};
    f.data[4] = 0xFF;
    TEST_ASSERT_TRUE(isFSDSelectedInUI(f));
}

// --- setSpeedProfileV12V13 ---

void test_setSpeedProfileV12V13_sets_profile_0()
{
    CanFrame f = {};
    f.data[6] = 0xFF;
    setSpeedProfileV12V13(f, 0);
    TEST_ASSERT_EQUAL_HEX8(0xF9, f.data[6]); // bits 1-2 cleared
}

void test_setSpeedProfileV12V13_sets_profile_1()
{
    CanFrame f = {};
    f.data[6] = 0x00;
    setSpeedProfileV12V13(f, 1);
    TEST_ASSERT_EQUAL_HEX8(0x02, f.data[6]);
}

void test_setSpeedProfileV12V13_sets_profile_2()
{
    CanFrame f = {};
    f.data[6] = 0x00;
    setSpeedProfileV12V13(f, 2);
    TEST_ASSERT_EQUAL_HEX8(0x04, f.data[6]);
}

void test_setSpeedProfileV12V13_preserves_other_bits()
{
    CanFrame f = {};
    f.data[6] = 0xF9; // bits 1-2 clear, rest set
    setSpeedProfileV12V13(f, 1);
    TEST_ASSERT_EQUAL_HEX8(0xFB, f.data[6]);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_setBit_sets_bit0_of_byte0);
    RUN_TEST(test_setBit_sets_bit7_of_byte0);
    RUN_TEST(test_setBit_sets_bit_in_byte5);
    RUN_TEST(test_setBit_sets_bit_in_byte7);
    RUN_TEST(test_setBit_clears_bit);
    RUN_TEST(test_setBit_does_not_affect_other_bytes);

    RUN_TEST(test_readMuxID_extracts_lower_3_bits);
    RUN_TEST(test_readMuxID_masks_upper_bits);
    RUN_TEST(test_readMuxID_zero);
    RUN_TEST(test_readMuxID_max_value);

    RUN_TEST(test_isFSDSelectedInUI_true_when_bit6_set);
    RUN_TEST(test_isFSDSelectedInUI_false_when_bit6_clear);
    RUN_TEST(test_isFSDSelectedInUI_ignores_other_bits);
    RUN_TEST(test_isFSDSelectedInUI_true_with_other_bits);

    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_0);
    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_1);
    RUN_TEST(test_setSpeedProfileV12V13_sets_profile_2);
    RUN_TEST(test_setSpeedProfileV12V13_preserves_other_bits);

    return UNITY_END();
}
