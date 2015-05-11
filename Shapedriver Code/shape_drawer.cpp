#include "constants.h"
#include "mbed.h"
#include <math.h>

static char work_buffer[SLICES][WIDTH];
static int (*height)(float d) = NULL;

static float deg2rad(float p){
    return 2 * M_PI * p / SLICES;    
}

static float rad2deg(float p){
    return p * SLICES / (2 * M_PI);    
}

static void step_i(float a, float b, int &i){
    float diff = abs(a - b);
    if(diff > SLICES / 2)
        i = a > b ? i + 1: i - 1;
    else
        i = a > b ? i - 1: i + 1; 
        
    i = (i == SLICES) ? 0 : i;
    i = (i == -1) ? SLICES - 1 : i;
}

void set_hfunc(int (*h)(float d)){
    height = h;    
}

static void erase(){
    for(int i = 0; i < SLICES; i++){
        for(int j = 0; j < WIDTH; j++){
            work_buffer[i][j] = 0x00;    
        }    
    }    
}

static void draw_circle(float x, float y, float r, float _h){  
    if( x == 0 && y == 0){  
        for(int i = 0; i < SLICES; i++){
            float h;
            
            if(height != NULL)
                h = height(1.0 * i / SLICES);
            else
                h = _h;
                
            work_buffer[i][(int)rint(r)] |= (0x01 << (int)rint(h));    
        }  
    } else {
        //r = 2 x cos p + 2 y sin p, p from 0 to 180
        for(int i = 0; i < SLICES / 2; i++){
            float p = deg2rad(i);
            float r = 2 * x * cos(p) + 2 * y * sin(p);
            float h;
            
            if(height != NULL)
                h = height(2.0 * i / SLICES);
            else
                h = _h;  
                
                    
            if(r >= 0){
                work_buffer[i][(int)rint(r)] |= (0x01 << (int)rint(h));    
            } else {
                work_buffer[(i + SLICES / 2) % SLICES][(int)rint(r)] |= (0x01 << (int)rint(h));    
            }  
        }  
    } 
}

void draw_circle(float x, float y, float r, float _h, bool fill){
    if(!fill){
        draw_circle(x, y, r, _h);
    } else {
        int a = rint(r);
        for(int i = a; i > 0; i--){
            draw_circle(x, y, i, _h);    
        }    
    }
}

void draw_point(float x, float y, float z, float r, bool fill){
    height = NULL;
    
    int h1 = (int)rint(z - r);
    int h2 = (int)rint(z + r);
    
    h1 = h1 < 0 ? 0 : h1;
    h1 = h1 >= HEIGHT ? HEIGHT - 1 : h1;
    
    h2 = h2 < 0 ? 0 : h2;
    h2 = h2 >= HEIGHT ? HEIGHT - 1 : h2;
    
    for(int i = h1; i < h2; i++){
        float r_b = sqrt(r*r - i*i);
        draw_circle(x, y, i, r_b, fill); 
    }  
}

#define VERT 1
#define HORZ 0

static void draw_zvert_line(float x, float y, float z1, float z2);
static void draw_orig_line(float x1, float x2, float z1, float z2, int ori);
static void draw_perp_line(float a1, float a2, float b, float z1, float z2, int ori);
static void draw_arb_line(float x1, float x2, float y1, float y2, float z1, float z2);

static void norm_angle(float &p){
    p = p < 0 ? p + 2 * M_PI : p;  
    p = p > 2 * M_PI ? p - 2 * M_PI : p;     
}

void draw_line(float x1, float y1, float z1, float x2, float y2, float z2){
    
    //Perfectly vertical depth lines
    if(x1 == x2 && y1 == y2){
        draw_zvert_line(x1, y1, z1, z2);    
    }
    
    //Perfectly horizontal lines through origin
    else if(y1 == 0 && y2 == 0){
        draw_orig_line(x1, x2, z1, z2, HORZ);        
    }
    
    //Perfectly vertical lines through origin
    else if(x1 == 0 && x2 == 0){
        draw_orig_line(y1, y2, z1, z2, VERT);        
    }
      
    //Perfectly horizontal lines
    else if(y1 == y2){
        draw_perp_line(x1, x2, y1, z1, z2, HORZ);   
    }
    
    //Perfectly vertical lines
    // r = b / r cos p
    else if(x1 == x2){
        draw_perp_line(y1, y2, x1, z1, z2, VERT);
    }
    
    //Arbitrary lines
    else {
        draw_arb_line(x1, x2, y1, y2, z1, z2);
    }     
}

static void draw_arb_line(float x1, float x2, float y1, float y2, float z1, float z2){
    // y = mx + b, 
    // r = b / (sin p - m cos p)
    // 
    // if m is close to infinity, better to use 
    // x = ny + c
    // r = c / (cos p - n sin p)
    
    float m_1 = 1.0*(y2 - y1)/(x2 - x1); 
    float m_2 = 1.0*(x2 - x1)/(y2 - y1);
    float d = sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
    float slope = (z2 - z1) / d;
    float m, p1, p2, b, h;
    int ori;
        
    //x = m_2 y + b is steeper than y = m_1 x + b
    if(abs(m_2) > abs(m_1)){
        m = m_1;
        b = y1 - m * x1;  
        ori = HORZ; 
    } else {
        m = m_2;    
        b = x1 - m * y1;
        ori = VERT;
    }
    
    p1 = atan2(y1, x1);
    p2 = atan2(y2, x2);
        
    norm_angle(p1);
    norm_angle(p2);
        
    int i1 = rad2deg(p1);
    int i2 = rad2deg(p2);
        
    while(true){
        float p = deg2rad(i1);
        float r = (ori == HORZ) ? b / (sin(p) - m * cos(p)) : b / (cos(p) - m * sin(p));
            
        float dx = r * cos(p);
        float dy = r * sin(p);
            
        float dd = sqrt((x1 - dx)*(x1 - dx) + (y1 - dy)*(y1 - dy));
        
        if(height != NULL)
            h = height(dd / d);
        else
            h = z1 + slope * d;
            
        if((int)rint(r) < WIDTH)
            work_buffer[i1][(int)rint(r)] |= (0x01 << (int)rint(h));
            
        if(i1 == i2)
            break;
                
        step_i(i1, i2, i1);
    }    
}

static void draw_perp_line(float a1, float a2, float b, float z1, float z2, int ori){
    float p1 = (ori == HORZ) ? atan2(b, a1) : atan2(a1, b);
    float p2 = (ori == HORZ) ? atan2(b, a2) : atan2(a2, b); 
    
    norm_angle(p1);
    norm_angle(p2);
    
    int i1 = rad2deg(p1);
    int i2 = rad2deg(p2);
    
    float m = (z2 - z1)/(a2 - a1);
    
    while(true){    
        float p = deg2rad(i1);
        float r = (ori == HORZ) ? b / sin(p) : b / cos(p);
        float d = (ori == HORZ) ? b / tan(p) : b * tan(p);
        float h;
        
        if(height != NULL)
            h = height(d / (a2 - a1));
        else
            h = z1 + m * (d - a1);        
        
        if((int)rint(r) < WIDTH)
            work_buffer[i1][(int)rint(r)] |= (0x01 << (int)rint(h));
        
        if(i1 == i2)
            break;
            
        step_i(i1, i2, i1);
    }    
}

static void draw_orig_line(float r1, float r2, float z1, float z2, int ori){
    int p1, p2;
    float m;
    
    if(ori == HORZ){
        p1 = 0;
        p2 = SLICES / 2 - 1;
        m = (1.0*z2 - 1.0*z1)/(1.0*r2 - 1.0*r1);
    } else {
        p1 = SLICES / 4 - 1;
        p2 = 3 * SLICES / 4 - 1;
        m = (1.0*z2 - 1.0*z1)/(1.0*r2 - 1.0*r1);    
    }
    
    int i = r1;
    while(true){
        float h;
        
        if(height != NULL)
            h = height(i / (r2 - r1));
        else
            h = z1 + m * (i - r1);
        
        if(i >= 0){
            work_buffer[p1][i] |= (0x01 << (int)rint(h));    
        } else if(i <= 0){
            work_buffer[p2][-i] |= (0x01 << (int)rint(h));  
        }
        
        if(i == r2)
            break;    
            
        printf("i: %d, h: %d\n", i, (int)rint(h));
            
        i = r1 > r2 ? i - 1 : i + 1;
    }     
}

static void draw_zvert_line(float x, float y, float z1, float z2){
    float r = sqrt(1.0*x*x + 1.0*y*y);
    float p = atan2(y, x);
    
    //set p to 0 - 2p range, and convert to slice index
    norm_angle(p);
    p = rad2deg(p);
    
    int i = z1;
    while(true){
        work_buffer[(int)rint(p)][(int)rint(r)] |= (0x01 << i);
        
        if(i == z2)
            break;
        
        step_i(z1, z2, i);
    }     
}

/*
    Moves shapes in work_buffer over to display_buffer in a format that accounts for
    displaced blades on display
*/
static void init_displacements();
static int displacements[HEIGHT];

void move_buffer(char (*buffer)[SLICES][WIDTH]){
    static bool initialized = false;
    
    if(!initialized){
        init_displacements();    
    }
    
    for(int i = 0; i < SLICES; i++){
        for(int j = 0; j < WIDTH; j++){
            char bit = 0x00;
            
            for(int h = 0; h < HEIGHT; h++){
                bit |= (work_buffer[(i + displacements[h]) % SLICES][j] & (0x01 << h)); 
            }
            
            (*buffer)[i][j] = bit;
        }    
    } 
    
    erase();
}

/*
    Initializes buffer used to adjust for vertical misalignment of display blades
*/
static void init_displacements(){
    
    for(int i = 0; i < HEIGHT; i++){
        switch(i){
            case 0: displacements[i] = 0;               break;
            case 1: displacements[i] = 4 * SLICES / 8;  break; 
            case 2: displacements[i] = 7 * SLICES / 8 - 5; break;
            case 3: displacements[i] = 3 * SLICES / 8 - 8;  break;
            case 4: displacements[i] = 6 * SLICES / 8 - 6;  break;
            case 5: displacements[i] = 2 * SLICES / 8 - 7;  break;
            case 6: displacements[i] = 5 * SLICES / 8 - 7;  break;
            case 7: displacements[i] = 1 * SLICES / 8 - 8;  break;
        }    
    }     
}



