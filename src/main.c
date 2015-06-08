#include <pebble.h>

Window *my_window;
TextLayer  *text_layer;

static uint32_t dataCacheSize = 100;
static uint32_t dataCacheIdx = 0;
static uint16_t * dataCache;
static const AccelSamplingRate SAMPLE_RATE = ACCEL_SAMPLING_100HZ;
static uint16_t highScore = 0; // TODO: Load saved high score
static uint16_t is_running = 0;

// isqrt_impl and isqrt provided by Siu Ching Pong on stackoverflow
uint32_t isqrt_impl(uint32_t const n, uint32_t const xk) {
    uint32_t const xk1 = (xk + n / xk) / 2;
    return (xk1 >= xk) ? xk : isqrt_impl(n, xk1);
}
uint32_t isqrt(uint64_t const n) {
    if (n == 0) return 0;
    if (n == 18446744073709551615ULL) return 4294967295U;
    return isqrt_impl(n, n);
}

// Caches data for later use from the accelerometer data service
static void cache_accel(AccelData * data, uint32_t num_samples) {
  if (dataCache == NULL)
     dataCache = malloc(sizeof(uint16_t) * dataCacheSize);
  else if (dataCacheIdx + num_samples > dataCacheSize)
    dataCache = realloc(dataCache, sizeof(uint16_t) * (dataCacheSize + num_samples) * 2);
  for (unsigned int i = 0; i < num_samples; dataCacheIdx++, i++)
    dataCache[dataCacheIdx] = isqrt(data[i].x * data[i].x + data[i].y * data[i].y + data[i].z * data[i].z);
}

// Start data recording
static void start(ClickRecognizerRef recognizer, void *context) {
  if (is_running)
    return;
  // Register event handler with 25 sample buffer
  accel_data_service_subscribe(25, cache_accel);
  is_running = 1;
  text_layer_set_text(text_layer, "Recording...\n\nShake, shake, shake!\n\n(press any button to finish)");
}

// Analyse data
static void finish(ClickRecognizerRef recognizer, void *context) {
  if (!is_running)
    return;
  is_running = 0;
  // De-register event handler
  accel_data_service_unsubscribe();
  uint16_t max = dataCache[0];
  for (unsigned int i = 1; i < dataCacheIdx; i++)
    if (dataCache[i] > max)
      max = dataCache[i];
  if (max > highScore) {
    static char score_text[79];
    snprintf(score_text, 79, "Congratulations! You have a new high score of %d.\n\nThe old high score was %d.", max, highScore);
    text_layer_set_text(text_layer, score_text);
  }
  else {
    static char score_text[62];
    snprintf(score_text, 62, "Good try. Your score was %d and the high score is %d.", max, highScore);
    text_layer_set_text(text_layer, score_text);
  }
}

// Setup button handling
void click_config_provider(Window *window) {
  // We start recording once they take their finger off the button,
  //  and stop recording once they push down on any button
  window_raw_click_subscribe(BUTTON_ID_DOWN, finish, start, NULL);
  window_raw_click_subscribe(BUTTON_ID_UP, finish, start, NULL);
  window_raw_click_subscribe(BUTTON_ID_SELECT, finish, start, NULL);
}

static void main_window_load(Window *window) {
  // Setup the text layer
  text_layer = text_layer_create(layer_get_bounds(window_get_root_layer(my_window)));
  text_layer_set_text(text_layer, "Welcome! To get started, press any button.");
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(text_layer));
  // Set Accelerometer to sample rate
  accel_service_set_sampling_rate(SAMPLE_RATE);
}

void handle_init(void) {
  my_window = window_create();
  // Setup main_window_load to run once the window loads
  window_set_window_handlers(my_window, (WindowHandlers){.load = main_window_load});
  window_set_click_config_provider(my_window, (ClickConfigProvider) click_config_provider);
  window_stack_push(my_window, true);
}

void handle_deinit(void) {
  text_layer_destroy(text_layer);
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
