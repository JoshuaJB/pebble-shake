#include <pebble.h>
#include <math.h> // For integer square root (isqrt)

// Constants
static const AccelSamplingRate SAMPLE_RATE = ACCEL_SAMPLING_100HZ;
static const uint16_t STARTING_ALLOCATION = 100;
enum states {
  RECORDING,
  FINISHED,
  PRERUN,
  NOMEM
};

// Function prototypes
static void start(ClickRecognizerRef recognizer, void *context);
static void finish(ClickRecognizerRef recognizer, void *context);
static void cache_accel(AccelData * data, uint32_t num_samples);

// Global variables
static Window * my_window;
static TextLayer  * text_layer;
static uint32_t dataCacheSize = 0;
static uint32_t dataCacheIdx = 0;
static uint16_t * dataCache;
static uint16_t highscore;
static enum states state = PRERUN;

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
  text_layer_set_text(text_layer, "Recording...\n\nShake away!\n\n(press any button to finish)");
}

// Analyse and dislay data
static void finish(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state != RECORDING)
    return;
  state = FINISHED;
  // De-register acceleration event handler
  accel_data_service_unsubscribe();
  // Find maximimum accelerometer reading
  uint16_t max = dataCache[0];
  for (unsigned int i = 1; i < dataCacheIdx; i++)
    if (dataCache[i] > max)
      max = dataCache[i];
  // Display appropriate message
  if (max > highscore) {
    static char score_text[117];
    snprintf(score_text, 117, "Congratulations! You have a new high score of %d.\n\nThe old high score was %d.\n\nTo play again, press any button.", max, highscore);
    text_layer_set_text(text_layer, score_text);
    // Save new high score
    highscore = max;
    persist_write_int(0, highscore);
  }
  else {
    static char score_text[96];
    snprintf(score_text, 96, "Good try. Your score was %d and the high score is %d.\n\nTo play again, press any button.", max, highscore);
    text_layer_set_text(text_layer, score_text);
  }
}

// Caches data for later use from the accelerometer data service
static void cache_accel(AccelData * data, uint32_t num_samples) {
  // Automatically grow the array as needed (similar to Java's ArrayList or C++'s Vector)
  if (dataCacheIdx + num_samples > dataCacheSize) {
    // Initially allocate STARTING_ALLOCATION number of slots
    uint16_t newSize;
    if (dataCacheSize == 0)
      newSize = STARTING_ALLOCATION;
    else
      newSize = dataCacheSize * 2;
    // Pebble's realloc implementation is not ISO-compliant so we have to use malloc and memcpy
    uint16_t * tempDataCache = malloc(sizeof(uint16_t) * newSize);
    // Make sure the allocation succeeded before we use the memory
    if (tempDataCache != NULL) {
      memcpy(tempDataCache, dataCache, sizeof(uint16_t) * dataCacheSize);
      free(dataCache);
      dataCache = tempDataCache;
      dataCacheSize = newSize;
    }
    // If we collected any usable data, just finish early
    else if (dataCacheIdx > 0) {
      APP_LOG(APP_LOG_LEVEL_WARNING, "Early termination of recording due to insufficient memory.");
      finish(NULL, NULL);
      return;
    }
    // Otherwise, display an error
    else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to cache any data due to low memory.");
      // De-register acceleration event handler
      accel_data_service_unsubscribe();
      // Prevent entry to start or finish
      state = NOMEM;
      // Tell user
      text_layer_set_text(text_layer, "Oh no!\n\nIt looks like your Pebble has insufficient memory available.\n\nTry removing some background applications and try again.");
      return;
    }
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
  text_layer_set_text(text_layer, "Welcome!\n\nTo get started, press any button.");
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(text_layer));
  // Set Accelerometer to sample rate
  accel_service_set_sampling_rate(SAMPLE_RATE);
  // Load initial high score
  highscore = persist_read_int(0);
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
  free(dataCache);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
