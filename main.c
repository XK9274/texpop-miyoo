#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define onionFont "/mnt/SDCARD/miyoo/app/Exo-2-Bold-Italic.ttf"

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Usage: %s duration \"text\" color font_size x y\n", argv[0]);
        return EXIT_FAILURE;
    }
	
    int duration_seconds = atoi(argv[1]);
    int duration = duration_seconds * 1000;
    char* text_str = argv[2];
    char* color_str = argv[3];
    int font_size = atoi(argv[4]);

	SDL_Color color;
	if (strcmp(color_str, "white") == 0) {
		color = (SDL_Color) {0xff, 0xff, 0xff};
	} else if (strcmp(color_str, "black") == 0) {
		color = (SDL_Color) {0x00, 0x00, 0x00};
	} else if (strcmp(color_str, "orange") == 0) {
		color = (SDL_Color) {0xff, 0xa5, 0x00};
	} else if (strcmp(color_str, "blue") == 0) {
		color = (SDL_Color) {0x00, 0x00, 0xff};
	} else if (strcmp(color_str, "green") == 0) {
		color = (SDL_Color) {0x00, 0xff, 0x00};
	} else if (strcmp(color_str, "red") == 0) {
		color = (SDL_Color) {0xff, 0x00, 0x00};
	} else if (strcmp(color_str, "yellow") == 0) {
		color = (SDL_Color) {0xff, 0xff, 0x00};
	} else if (strcmp(color_str, "purple") == 0) {
		color = (SDL_Color) {0x80, 0x00, 0x80};
	} else if (strlen(color_str) == 6) {
		unsigned int hex_color;
		if (sscanf(color_str, "%x", &hex_color) == 1) {
			color = (SDL_Color) {(hex_color >> 16) & 0xff, (hex_color >> 8) & 0xff, hex_color & 0xff};
		} else {
			printf("Error: Invalid color specified\n");
			return EXIT_FAILURE;
		}
	} else {
		printf("Error: Invalid color specified\n");
		return EXIT_FAILURE;
	}
		
   if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);

    if (TTF_Init() != 0) {
        printf("Error: Unable to initialize SDL_ttf: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }
    atexit(TTF_Quit);

	// Load the font
	char* path = NULL;
	if (access(onionFont, F_OK) == 0) path = onionFont;
	if (!path) return 0;
	TTF_Font* font = TTF_OpenFont(path, font_size);
	if (!font) {
		printf("Error: Unable to load font: %s\n", TTF_GetError());
		return EXIT_FAILURE;
	}
	
	TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

	// Render the text
	SDL_Surface* text = TTF_RenderText_Blended(font, text_str, color);
	if (!text) {
		printf("Error: Unable to render text: %s\n", TTF_GetError());
		TTF_CloseFont(font);
		return EXIT_FAILURE;
	}

    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Error: Unable to open framebuffer device");
        SDL_FreeSurface(text);
        TTF_CloseFont(font);
        return EXIT_FAILURE;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error: Unable to get variable screen information");
        close(fb_fd);
        SDL_FreeSurface(text);
        TTF_CloseFont(font);
        return EXIT_FAILURE;
    }

	int width = vinfo.xres;
    int height = vinfo.yres;
    int bpp = vinfo.bits_per_pixel;
    int pitch = width * (bpp / 8);
    size_t map_size = pitch * height;
    void* fb0_map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    SDL_Surface* screen = SDL_CreateRGBSurfaceFrom(fb0_map, width, height, bpp, pitch, 0, 0, 0, 0);

	int x = atoi(argv[5]);
    int y = atoi(argv[6]);
    SDL_Rect dst_rect = {x, y, text->w, text->h};

	SDL_Surface* rotated_text = rotozoomSurface(text, 180, 1, 1);
	if (!rotated_text) {
        fprintf(stderr, "Error: Unable to rotate the text: %s\n", SDL_GetError());
        SDL_FreeSurface(text);
		TTF_CloseFont(font);
		return EXIT_FAILURE;
	}
    SDL_FreeSurface(text);
    text = rotated_text;

	Uint32 start_time = SDL_GetTicks();
    Uint32 elapsed_time = 0;

    while (elapsed_time < duration) {
    SDL_BlitSurface(text, NULL, screen, &dst_rect);
    elapsed_time = SDL_GetTicks() - start_time;
    SDL_Delay(1000);
	}

    // Clean up
    munmap(fb0_map, map_size);
    close(fb_fd);
    SDL_FreeSurface(text);
    TTF_CloseFont(font);

    return 0;
}