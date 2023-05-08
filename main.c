#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define onionFont "/mnt/SDCARD/miyoo/app/Exo-2-Bold-Italic.ttf"

static void blit(void* _dst, int dst_w, int dst_h, void* _src, int src_w, int src_h, int ox, int oy) {
    uint8_t* dst = (uint8_t*)_dst;
    uint8_t* src = (uint8_t*)_src;

    for (int y=0; y<src_h; y++) {
        uint8_t* dst_row = dst + (((((dst_h - 1 - oy) - y) * dst_w) - 1 - ox) * 4);
        uint8_t* src_row = src + ((y * src_w) * 4);
        for (int x=0; x<src_w; x++) {
            float a = *(src_row+3) / 255.0;
            if (a>0.8) {
                *(dst_row+0) = *(src_row+0) * a;
                *(dst_row+1) = *(src_row+1) * a;
                *(dst_row+2) = *(src_row+2) * a;
                *(dst_row+3) = 0xff;
            }
            dst_row -= 4;
            src_row += 4;
        }
    }
}

Uint32 get_pixel(SDL_Surface* surface, int x, int y);
void put_pixel(SDL_Surface* surface, int x, int y, Uint32 pixel);

SDL_Surface* flip_surface(SDL_Surface* src) {
    SDL_Surface* dest = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (!dest) {
        return NULL;
    }

    for (int y = 0; y < src->h; ++y) {
        for (int x = 0; x < src->w; ++x) {
            int flipped_x = src->w - x - 1;
            int flipped_y = src->h - y - 1;
            Uint32 pixel = get_pixel(src, x, y);
            put_pixel(dest, flipped_x, flipped_y, pixel);
        }
    }

    return dest;
}

Uint32 get_pixel(SDL_Surface* surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
        case 2:
            return *(Uint16*)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                return p[0] << 16 | p[1] << 8 | p[2];
            } else {
                return p[0] | p[1] << 8 | p[2] << 16;
            }
        case 4:
            return *(Uint32*)p;
        default:
            return 0;
    }
}

void put_pixel(SDL_Surface* surface, int x, int y, Uint32 pixel) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            *p = pixel;
            break;
        case 2:
            *(Uint16*)p = pixel;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;
        case 4:
            *(Uint32*)p = pixel;
            break;
    }
}

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


	// Determine the color to use
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
		
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: Unable to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    atexit(SDL_Quit);

    // Initialize TTF
    if (TTF_Init() != 0) {
        printf("Error: Unable to initialize SDL_ttf: %s\n", TTF_GetError());
        return EXIT_FAILURE;
    }
    atexit(TTF_Quit);

	// Load the font
	char* path = NULL;
	if (access(onionFont, F_OK) == 0) path = onionFont;
	if (!path) return 0;
	TTF_Font* font = TTF_OpenFont(path, font_size); // Set font size to 40 pixels
	if (!font) {
		printf("Error: Unable to load font: %s\n", TTF_GetError());
		return EXIT_FAILURE;
	}

	// Render the text
	SDL_Surface* text = TTF_RenderText_Blended(font, text_str, color);
	if (!text) {
		printf("Error: Unable to render text: %s\n", TTF_GetError());
		TTF_CloseFont(font);
		return EXIT_FAILURE;
	}

    // Set the surface alpha blending mode
    SDL_SetAlpha(text, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);

    // Open the framebuffer device
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Error: Unable to open framebuffer device");
        SDL_FreeSurface(text);
        TTF_CloseFont(font);
        return EXIT_FAILURE;
    }

    // Get framebuffer information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error: Unable to get variable screen information");
        close(fb_fd);
        SDL_FreeSurface(text);
        TTF_CloseFont(font);
        return EXIT_FAILURE;
    }

    // Create an SDL surface from the framebuffer
    int width = vinfo.xres;
    int height = vinfo.yres;
    int bpp = vinfo.bits_per_pixel;
    int pitch = width * (bpp / 8);
    size_t map_size = pitch * height;
    void* fb0_map = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    SDL_Surface* screen = SDL_CreateRGBSurfaceFrom(fb0_map, width, height, bpp, pitch, 0, 0, 0, 0);

	// Render the text onto the framebuffer surface
	int x = atoi(argv[5]);
	int y = atoi(argv[6]);
	SDL_Rect dst_rect = {x, y, text->w, text->h};

	SDL_Surface* flipped_text = flip_surface(text);
	if (!flipped_text) {
		fprintf(stderr, "Error: Unable to flip the text: %s\n", SDL_GetError());
		SDL_FreeSurface(text);
		TTF_CloseFont(font);
		return EXIT_FAILURE;
	}
	SDL_FreeSurface(text);
	text = flipped_text;

	Uint32 start_time = SDL_GetTicks();
	Uint32 elapsed_time = 0;

	while (elapsed_time < duration) {
		SDL_BlitSurface(text, NULL, screen, &dst_rect);
		elapsed_time = SDL_GetTicks() - start_time;
	}

	// Clean up
	munmap(fb0_map, map_size);
	close(fb_fd);
	SDL_FreeSurface(text);
	TTF_CloseFont(font);

	return 0;
}