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
static uint16_t highscore;
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
  /* TODO: Register acceleration event handler with a 25 sample buffer
   * See: http://developer.getpebble.com/docs/c/Foundation/Event_Service/AccelerometerService/
   */
  
  /* TODO: Display the pre-run message
   * Hint: Use text_layer_set_text(TextLayer * <layer>, char * <text>))
   * See: http://developer.getpebble.com/docs/c/User_Interface/Layers/TextLayer/#text_layer_set_text
   */
  
}

// Analyse and dislay data
static void finish(ClickRecognizerRef recognizer, void *context) {
  // Manage the state
  if (state != RECORDING)
    return;
  state = FINISHED;
  /* TODO: De-register acceleration event handler
   * See: http://developer.getpebble.com/docs/c/Foundation/Event_Service/AccelerometerService/
   */
  
  /* TODO: Find maximimum accelerometer reading
   * Hint: You only need one variable)
   */

  /* TODO: If the maximum reading is larger than the highscore,
   *       print a congratulations and the new highscore. Then
   *       save it with persist_write_int(0, highscore). Other-
   *       -wise, tell the user that their score wasn't good
   *       enough to beat the highscore.
   * Hint: Use snprintf, text_layer_set_text, and make sure that
   *       you pass a static char pointer
   * See: http://developer.getpebble.com/docs/c/Standard_C/Format/
   *      http://developer.getpebble.com/docs/c/Foundation/Storage/#persist_write_int
   *      http://developer.getpebble.com/docs/c/User_Interface/Layers/TextLayer/#text_layer_set_text
   */
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
  /* TODO: Compute each the acceleration vector magnitude and
   *       store it in dataCache.
   * Hint: Use isqrt and the pythagorian theorem
   * See: http://developer.getpebble.com/docs/c/Foundation/Event_Service/AccelerometerService/#AccelData
   */

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
  /* TODO: Set Accelerometer to sample rate
   * Hint: Use the SAMPLE_RATE constant defined on line 14
   * See: http://developer.getpebble.com/docs/c/Foundation/Event_Service/AccelerometerService/#accel_service_set_sampling_rate
   */

  /* TODO: Load the initial high score, use a key of 0
   * Hint: highscore is already setup as a global variable
   * See: http://developer.getpebble.com/docs/c/Foundation/Storage/#persist_read_int
   */

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
