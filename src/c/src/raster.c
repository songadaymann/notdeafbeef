#include "raster.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void raster_clear(uint32_t *fb, int w, int h, uint32_t color)
{
    for(int i=0;i<w*h;i++) fb[i]=color;
}

static inline void plot(uint32_t *fb,int w,int h,int x,int y,uint32_t col){
    if((unsigned)x<(unsigned)w && (unsigned)y<(unsigned)h) fb[y*w+x]=col;
}

void raster_circle(uint32_t *fb,int w,int h,int cx,int cy,int r,uint32_t col,int thickness)
{
    if(thickness<=0) thickness=1;
    int r_out=r;
    int r_in=r-thickness;
    int r_out2=r_out*r_out;
    int r_in2=r_in*r_in;
    for(int y=-r_out;y<=r_out;y++){
        int yy=y+cy;
        int y2=y*y;
        for(int x=-r_out;x<=r_out;x++){
            int xx=x+cx;
            int dist2 = x*x + y2;
            if(dist2<=r_out2 && dist2>=r_in2){ plot(fb,w,h,xx,yy,col);}        }
    }
}

/* Filled circle using simple scanline fill */
void raster_fill_circle(uint32_t *fb,int w,int h,int cx,int cy,int r,uint32_t col)
{
    int r2 = r*r;
    for(int y=-r; y<=r; ++y){
        int yy = cy + y;
        if((unsigned)yy >= (unsigned)h) continue;
        int x_extent = (int)sqrtf((float)(r2 - y*y));
        int x_min = cx - x_extent;
        int x_max = cx + x_extent;
        if(x_min < 0) x_min = 0;
        if(x_max >= w) x_max = w-1;
        for(int x = x_min; x <= x_max; ++x){
            fb[yy*w + x] = col;
        }
    }
}

/* Bresenham line (thickness = 1) */
void raster_line(uint32_t *fb,int w,int h,int x0,int y0,int x1,int y1,uint32_t col)
{
    int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    while(true){
        plot(fb, w, h, x0, y0, col);
        if(x0 == x1 && y0 == y1) break;
        e2 = 2*err;
        if(e2 >= dy){ err += dy; x0 += sx; }
        if(e2 <= dx){ err += dx; y0 += sy; }
    }
}

/* Simple polygon: if fill==true, use scanline fill; otherwise draw outline with raster_line */
void raster_poly(uint32_t *fb,int w,int h,const int *vx,const int *vy,int n,uint32_t col,bool fill,int thickness)
{
    if(n < 2) return;
    if(!fill){
        for(int i=0;i<n;i++){
            int j = (i+1)%n;
            raster_line(fb,w,h,vx[i],vy[i],vx[j],vy[j],col);
            if(thickness>1){ /* naive thickness by drawing parallel lines */
                for(int t=1;t<thickness;t++){
                    raster_line(fb,w,h,vx[i]+t,vy[i],vx[j]+t,vy[j],col);
                    raster_line(fb,w,h,vx[i]-t,vy[i],vx[j]-t,vy[j],col);
                }
            }
        }
        return;
    }

    /* --- fill --- */
    /* Determine bounding box */
    int y_min = vy[0], y_max = vy[0];
    for(int i=1;i<n;i++){
        if(vy[i] < y_min) y_min = vy[i];
        if(vy[i] > y_max) y_max = vy[i];
    }
    if(y_min < 0) y_min = 0;
    if(y_max >= h) y_max = h-1;

    for(int y=y_min; y<=y_max; ++y){
        /* Build list of x intersections with edges */
        int inter[32]; /* assume n<=32 */
        int count = 0;
        for(int i=0;i<n;i++){
            int j = (i+1)%n;
            int y0 = vy[i], y1 = vy[j];
            if((y0<y && y1>=y) || (y1<y && y0>=y)){
                int x0 = vx[i], x1 = vx[j];
                /* linear interpolation */
                int x = x0 + (int)((float)(y - y0) * (float)(x1 - x0) / (float)(y1 - y0 + (y1==y0))); /* avoid div0 */
                if(count < 32){ inter[count++] = x; }
            }
        }
        if(count < 2) continue;
        /* sort small array (insertion) */
        for(int i=1;i<count;i++){
            int key=inter[i]; int j=i-1; while(j>=0 && inter[j]>key){ inter[j+1]=inter[j]; j--; } inter[j+1]=key; }
        for(int i=0;i<count; i+=2){
            int x_start = inter[i];
            int x_end   = inter[i+1];
            if(x_start < 0) x_start = 0;
            if(x_end >= w) x_end = w-1;
            for(int x=x_start; x<=x_end; ++x){
                fb[y*w + x] = col;
            }
        }
    }
}

/* Blit helper: copy src_w*src_h pixels at (dx,dy) into dst, no alpha */
void raster_blit_rgba(const uint32_t *src,int src_w,int src_h,uint32_t *dst,int dst_w,int dst_h,int dx,int dy)
{
    for(int y=0;y<src_h;y++){
        int dst_y = dy + y;
        if((unsigned)dst_y >= (unsigned)dst_h) continue;
        const uint32_t *srow = src + y*src_w;
        uint32_t *drow = dst + dst_y*dst_w;
        int copy_w = src_w;
        int dst_x = dx;
        if(dst_x < 0){ srow += -dst_x; copy_w += dst_x; dst_x = 0; }
        if(dst_x + copy_w > dst_w) copy_w = dst_w - dst_x;
        if(copy_w <= 0) continue;
        memcpy(drow + dst_x, srow, copy_w * sizeof(uint32_t));
    }
}

void raster_blit_rgba_alpha(const uint32_t *src,int src_w,int src_h,uint32_t *dst,int dst_w,int dst_h,int dx,int dy)
{
    for(int y=0;y<src_h;y++){
        int dst_y = dy + y;
        if((unsigned)dst_y >= (unsigned)dst_h) continue;
        const uint32_t *srow = src + y*src_w;
        uint32_t *drow = dst + dst_y*dst_w;
        for(int x=0; x<src_w; ++x){
            int dst_x = dx + x;
            if((unsigned)dst_x >= (unsigned)dst_w) continue;
            uint32_t px = srow[x];
            if((px & 0xFF) == 0) continue; /* alpha==0 skip */
            drow[dst_x] = px;
        }
    }
} 