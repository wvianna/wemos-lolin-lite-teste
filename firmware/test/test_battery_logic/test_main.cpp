// Testes HOST da lógica pura (env `native`).
// Cobre CA-002 (fator de escala 2,0) e CA-003 (formatação exata da mensagem).
#include <cstring>

#include <unity.h>

#include "battery_logic.h"

void setUp(void) {}
void tearDown(void) {}

// --- FR-002 / CA-002: fator de reconstrução de escala ---------------------

static void test_scale_factor_is_exactly_two(void) {
  TEST_ASSERT_EQUAL_FLOAT(2.0f, battery::kDividerScaleFactor);
}

static void test_zero_millivolts_maps_to_zero_volts(void) {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, battery::batteryVolts(0));
}

static void test_converted_values(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.2f, battery::batteryVolts(2100));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, battery::batteryVolts(1500));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f, battery::batteryVolts(1250));
}

static void test_negative_input_is_clamped_to_zero(void) {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, battery::batteryVolts(-1));
}

// --- FR-002 / NFR-002: média móvel ---------------------------------------

static void test_averager_empty_returns_zero(void) {
  battery::MilliVoltAverager averager;
  averager.reset();
  TEST_ASSERT_EQUAL_UINT(0U, static_cast<unsigned>(averager.count()));
  TEST_ASSERT_EQUAL_INT32(0, averager.averageMilliVolts());
}

static void test_averager_partial_window(void) {
  battery::MilliVoltAverager averager;
  averager.reset();
  averager.add(1000);
  averager.add(2000);
  TEST_ASSERT_EQUAL_UINT(2U, static_cast<unsigned>(averager.count()));
  TEST_ASSERT_EQUAL_INT32(1500, averager.averageMilliVolts());
}

static void test_averager_full_window_and_discards_oldest(void) {
  battery::MilliVoltAverager averager;
  averager.reset();
  for (int i = 0; i < 8; ++i) {
    averager.add(2000);
  }
  TEST_ASSERT_EQUAL_UINT(8U, static_cast<unsigned>(averager.count()));
  TEST_ASSERT_EQUAL_INT32(2000, averager.averageMilliVolts());

  // A 9ª amostra descarta a mais antiga: (7 * 2000 + 4000) / 8 = 2250.
  averager.add(4000);
  TEST_ASSERT_EQUAL_UINT(8U, static_cast<unsigned>(averager.count()));
  TEST_ASSERT_EQUAL_INT32(2250, averager.averageMilliVolts());
}

static void test_averager_reset_clears_state(void) {
  battery::MilliVoltAverager averager;
  averager.reset();
  averager.add(3300);
  averager.reset();
  TEST_ASSERT_EQUAL_UINT(0U, static_cast<unsigned>(averager.count()));
  TEST_ASSERT_EQUAL_INT32(0, averager.averageMilliVolts());
}

// --- FR-003 / CA-003: mensagem serial ------------------------------------

static void test_message_matches_specification(void) {
  char buffer[48];
  const int written = battery::formatBatteryMessage(4.2f, buffer, sizeof(buffer));

  TEST_ASSERT_EQUAL_STRING("Tensão da Bateria: 4.20 V", buffer);
  TEST_ASSERT_EQUAL_UINT(static_cast<unsigned>(std::strlen(buffer)),
                         static_cast<unsigned>(written));
}

static void test_message_always_has_two_decimals(void) {
  char buffer[48];

  battery::formatBatteryMessage(0.0f, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("Tensão da Bateria: 0.00 V", buffer);

  battery::formatBatteryMessage(3.0f, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("Tensão da Bateria: 3.00 V", buffer);

  battery::formatBatteryMessage(3.999f, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("Tensão da Bateria: 4.00 V", buffer);

  battery::formatBatteryMessage(4.196f, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_STRING("Tensão da Bateria: 4.20 V", buffer);
}

static void test_message_rejects_small_buffer(void) {
  char buffer[8];
  TEST_ASSERT_EQUAL_INT(-1, battery::formatBatteryMessage(4.2f, buffer, sizeof(buffer)));
}

static void test_message_rejects_null_buffer(void) {
  TEST_ASSERT_EQUAL_INT(-1, battery::formatBatteryMessage(4.2f, nullptr, 0));
}

// --- Execução ------------------------------------------------------------

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scale_factor_is_exactly_two);
  RUN_TEST(test_zero_millivolts_maps_to_zero_volts);
  RUN_TEST(test_converted_values);
  RUN_TEST(test_negative_input_is_clamped_to_zero);
  RUN_TEST(test_averager_empty_returns_zero);
  RUN_TEST(test_averager_partial_window);
  RUN_TEST(test_averager_full_window_and_discards_oldest);
  RUN_TEST(test_averager_reset_clears_state);
  RUN_TEST(test_message_matches_specification);
  RUN_TEST(test_message_always_has_two_decimals);
  RUN_TEST(test_message_rejects_small_buffer);
  RUN_TEST(test_message_rejects_null_buffer);
  return UNITY_END();
}
