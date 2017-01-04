#include "../inc/brightness.h"

static void get_max_brightness(void);
static void get_current_brightness(void);
static void free_bus_structs(sd_bus_error *err, sd_bus_message *m);

struct brightness {
    int current;
    int max;
    int old;
};

static struct brightness br;
static sd_bus *bus;

void init_brightness(void) {
    int r;

    /* Connect to the system bus */
    r = sd_bus_open_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        quit = 1;
        return;
    }
    
    get_max_brightness();
    get_current_brightness();
}

static void get_max_brightness(void) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    int r;
    r = sd_bus_call_method(bus,
                           "org.clight.backlight",
                           "/org/clight/backlight",
                           "org.clight.backlight",
                           "getmaxbrightness",
                           &error,
                           &m,
                           "s",
                           conf.screen_path);
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        quit = 1;
        goto finish;
    }
    
    /* Parse the response message */
    r = sd_bus_message_read(m, "i", &br.max);
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        quit = 1;
        goto finish;
    }
    
finish:
    free_bus_structs(&error, m);
}

static void get_current_brightness(void) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    int r;
    r = sd_bus_call_method(bus,
                           "org.clight.backlight",
                           "/org/clight/backlight",
                           "org.clight.backlight",
                           "getbrightness",
                           &error,
                           &m,
                           "s",
                           conf.screen_path);
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        quit = 1;
        goto finish;
    }
    
    /* Parse the response message */
    r = sd_bus_message_read(m, "i", &br.current);
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        quit = 1;
        goto finish;
    }
    
finish:
    free_bus_structs(&error, m);
}

double set_brightness(double perc) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    int r;
    
    br.old = br.current;
    
    /* Issue the method call and store the response message in m */
    r = sd_bus_call_method(bus,
                           "org.clight.backlight",
                           "/org/clight/backlight",
                           "org.clight.backlight",
                           "setbrightness",
                           &error,
                           &m,
                           "si",
                           conf.screen_path,
                           (int) (br.max * perc));
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        quit = 1;
        goto finish;
    }
    
    /* Parse the response message */
    r = sd_bus_message_read(m, "i", &br.current);
    printf("New brightness value: %d\n", br.current);
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        quit = 1;
        goto finish;
    }
    
finish:
    free_bus_structs(&error, m);
    return (double)(br.current - br.old) / br.max;

}

double capture_frames(void) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    int r;
    double avg_brightness = 0.0;
        
    /* Issue the method call and store the response message in m */
    r = sd_bus_call_method(bus,
                           "org.clight.backlight",
                           "/org/clight/backlight",
                           "org.clight.backlight",
                           "captureframes",
                           &error,
                           &m,
                           "si",
                           conf.dev_name,
                           conf.num_captures);
    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        quit = 1;
        goto finish;
    }
    
    /* Parse the response message */
    r = sd_bus_message_read(m, "d", &avg_brightness);
    if (r < 0) {
        fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        quit = 1;
        goto finish;
    }
    
finish:
    free_bus_structs(&error, m);
    return avg_brightness;
}

static void free_bus_structs(sd_bus_error *err, sd_bus_message *m) {
    if (err) {
        sd_bus_error_free(err);
    }
    
    if (m) {
        sd_bus_message_unref(m);
    }
}

void free_brightness(void) {
    sd_bus_flush_close_unref(bus);
}