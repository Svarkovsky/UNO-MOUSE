#include <TVout.h>
#include <avr/pgmspace.h> // For pgm_read_byte, pgm_read_word_near

TVout TV;

#include "images.h"

// Array of pointers to compressed images (in Flash)
const unsigned char * const image_list_compressed[] PROGMEM = {
  Lenna_compressed,
  input_image_compressed,
  cockatoos_compressed,
  _4_compressed,
};

// Number of images (in Flash)
const int num_images PROGMEM = 4;

// --- Function to draw RLE compressed image directly to TVout buffer ---
// Takes a pointer to compressed data in Flash
void draw_rle_bitmap_direct(int start_x, int start_y, const unsigned char* compressed_bmp) {
  // Read decompressed size (2 bytes) and compressed size (2 bytes)
  uint16_t decompressed_size_bytes = pgm_read_word_near(compressed_bmp); // Expected 1536
  uint16_t compressed_size = pgm_read_word_near(compressed_bmp + 2);

  // Pointer to the start of compressed RLE data (after sizes)
  const unsigned char* rle_data_ptr = compressed_bmp + 4;

  int current_pixel_x = start_x;
  int current_pixel_y = start_y;
  int pixels_drawn = 0; // Counter for drawn pixels (to track position)
  int total_pixels_expected = decompressed_size_bytes * 8; // 128 * 96 = 12288 pixels

  // TVout buffer width in bytes (128 / 8 = 16)
  const int TVOUT_BUFFER_BYTE_WIDTH = 128 / 8;

  // Read and decompress RLE data
  for (uint16_t i = 0; i < compressed_size; ++i) {
    // Read RLE pair byte from Flash
    unsigned char rle_byte = pgm_read_byte_near(rle_data_ptr + i);

    // Extract count (lower 7 bits)
    int count = rle_byte & 0x7F;
    // Extract pixel value (most significant bit: 0 for black, 1 for white)
    int pixel_value = (rle_byte >> 7) & 0x01; // 0 or 1

    // Determine byte pattern for setting bits (all 8 bits are the same)
    unsigned char bit_pattern = (pixel_value == 1) ? 0xFF : 0x00;

    // Draw count pixels with pixel_value
    for (int j = 0; j < count; ++j) {
      // Check if we exceeded the expected number of pixels
      if (pixels_drawn < total_pixels_expected) {
         // Calculate byte index in the TVout buffer
         int byte_idx = (current_pixel_x / 8) + (current_pixel_y * TVOUT_BUFFER_BYTE_WIDTH);
         // Calculate bit mask within the byte
         int bit_mask = 0x80 >> (current_pixel_x & 7);

         // Set the pixel directly in the TVout buffer (TV.screen)
         if (pixel_value == 1) {
             TV.screen[byte_idx] |= bit_mask; // Set bit to 1 (white)
         } else {
             TV.screen[byte_idx] &= ~bit_mask; // Set bit to 0 (black)
         }

         // Move to the next pixel
         current_pixel_x++;
         pixels_drawn++;

         // If we reached the end of the image row (128 pixels), move to the next row
         if (current_pixel_x >= start_x + 128) {
           current_pixel_x = start_x;
           current_pixel_y++;
           // If we exceeded the image height (96 rows), stop
           if (current_pixel_y >= start_y + 96) {
             // Reached the end of the image, can exit loops
             goto end_decompression; // Use goto to exit nested loops
           }
         }
      } else {
          // All expected pixels drawn, exit
          goto end_decompression;
      }
    }
  }

end_decompression:; // Label for goto
}


// --- Melody ---
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6 1047
#define NOTE_D6 1175
#define NOTE_E6 1319

// Melody and durations also in Flash (doubled length)
const int melody[] PROGMEM = {
  NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5,
  NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6, NOTE_D6, NOTE_C6, NOTE_B5, NOTE_A5,
  NOTE_B5, NOTE_C6, NOTE_D6, NOTE_E6, NOTE_E6, NOTE_D6, NOTE_C6, NOTE_B5,
  NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5,
  NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6, NOTE_D6, NOTE_C6, NOTE_B5, NOTE_A5,
  NOTE_B5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D6, NOTE_C6, NOTE_B5,
  // Repeat melody for increased duration
  NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5,
  NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6, NOTE_D6, NOTE_C6, NOTE_B5, NOTE_A5,
  NOTE_B5, NOTE_C6, NOTE_D6, NOTE_E6, NOTE_E6, NOTE_D6, NOTE_C6, NOTE_B5,
  NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5,
  NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_C6, NOTE_B5, NOTE_A5, NOTE_G5,
  NOTE_A5, NOTE_B5, NOTE_C6, NOTE_D6, NOTE_D6, NOTE_C6, NOTE_B5, NOTE_A5,
  NOTE_B5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D6, NOTE_C6, NOTE_B5
};

const int noteDurations[] PROGMEM = {
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  // Repeat durations for doubled melody
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2
};


void play() {
  // Read melody and duration data from Flash memory
  for (int thisNote = 0; thisNote < sizeof(melody)/sizeof(melody[0]); thisNote++) {
    int base_duration = 120;
    // Read note duration from Flash
    int duration_val = pgm_read_word_near(noteDurations + thisNote);
    int duration_ms = base_duration * (duration_val / 2);
    
    // Read note frequency from Flash
    int note_freq = pgm_read_word_near(melody + thisNote);
    TV.tone(note_freq, duration_ms);

    int pauseBetweenNotes = duration_ms + 20;
    TV.delay(pauseBetweenNotes);

    // Stop tone before the next note
    TV.noTone();
  }
}

void setup() {
  // Initialize TVout with target resolution
  TV.begin(NTSC, 128, 96);
  // TV.begin(PAL, 128, 96); // For PAL region

  // --- Adjust horizontal offset ---
  // Default value can be around 10-15.
  // Try starting with 10 or 12 and increase/decrease
  // until the image is centered. 14 is a starting value.
  TV.force_outstart(14); // <-- Starting value
  // ---------------------------------------------

  TV.clear_screen(); // Clear screen
}

void loop() {
  static int current_image = 0;
  
  TV.clear_screen();
  
  // Read pointer to the current compressed image from Flash
  const unsigned char *current_image_compressed_ptr = (const unsigned char *)pgm_read_word_near(image_list_compressed + current_image);
  
  // Display the RLE compressed image at position (0,0)
  draw_rle_bitmap_direct(0, 0, current_image_compressed_ptr);
  
  // Play the melody
  play();
  
  // Delay
  TV.delay(3000);
  
  // Move to the next image
  // Read num_images from Flash
  int total_images = pgm_read_word_near(&num_images);
  current_image = (current_image + 1) % total_images;
}
