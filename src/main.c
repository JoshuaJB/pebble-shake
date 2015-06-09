#include <pebble.h>
#include <math.h> // For integer square root (isqrt)

static void start(ClickRecognizerRef recognizer, void *context);
static void finish(ClickRecognizerRef recognizer, void *context);
static void cache_accel(AccelData * data, uint32_t num_samples);

Window *my_window;
TextLayer  *text_layer;

static uint32_t dataCacheSize = 100;
static uint32_t dataCacheIdx = 0;
static uint16_t * dataCache;
static const AccelSamplingRate SAMPLE_RATE = ACCEL_SAMPLING_100HZ;
static uint16_t highScore;
enum states {
  RECORDING,
  FINISHED,
  PRERUN,
  NOMEM
};
enum states state = PRERUN;

// Start data recording
static void start(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state == FINISHED) {
    // Reset the data cache
    dataCacheIdx = 0;
    // Next state will be recording
    state = PRERUN;
    return;
  }
  else if (state != PRERUN)
    return;
  state = RECORDING;
  // Register acceleration event handler with a 25 sample buffer
  accel_data_service_subscribe(25, cache_accel);
  // Display the pre-run message
  text_layer_set_text(text_layer, "Recording...\n\nShake, shake, shake!\n\n(press any button to finish)");
}

// Analyse and dislay data
static void finish(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state != RECORDING)
    return;
  state = FINISHED;
  // De-register acceleration event handler
  accel_data_service_unsubscribe();
  // Analyse and display
  uint16_t max = dataCache[0];
  for (unsigned int i = 1; i < dataCacheIdx; i++)
    if (dataCache[i] > max)
      max = dataCache[i];
  if (max > highScore) {
    static char score_text[83];
    snprintf(score_text, 83, "Congratulations! You have a new high score of %d.\n\nThe old high score was %d.", max, highScore);
    text_layer_set_text(text_layer, score_text);
    // Save new high score
    highScore = max;
    persist_write_int(0, highScore);
  }
  else {
    static char score_text[62];
    snprintf(score_text, 62, "Good try. Your score was %d and the high score is %d.", max, highScore);
    text_layer_set_text(text_layer, score_text);
  }
}

// Caches data for later use from the accelerometer data service
static void cache_accel(AccelData * data, uint32_t num_samples) {
  // We automatically grow the array as needed
  if (dataCache == NULL) {
    // Fail gracefully if the initial memory allocation fails
    if ((dataCache = malloc(sizeof(uint16_t) * dataCacheSize)) == NULL) {
      text_layer_set_text(text_layer, "Oh no!\n\nIt looks like your Pebble has insufficient memory. Try removing any background applications and try again.");
      // De-register acceleration event handler
      accel_data_service_unsubscribe();
      state = NOMEM;
    }
  }
  else if (dataCacheIdx + num_samples > dataCacheSize) {
    uint16_t * tempDataCache = realloc(dataCache, sizeof(uint16_t) * (dataCacheSize + num_samples) * 2);
    // If the memory allocation failed, just analyse what data we were able to collect.
    if (tempDataCache == NULL)
      finish(NULL, NULL);
    else
      dataCache = tempDataCache;
  }
  // Compute the acceleration vector magnitude and store it.
  for (unsigned int i = 0; i < num_samples; dataCacheIdx++, i++)
    dataCache[dataCacheIdx] = isqrt(data[i].x * data[i].x + data[i].y * data[i].y + data[i].z * data[i].z);
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
  // Load initial high score
  highScore = persist_read_int(0);
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
