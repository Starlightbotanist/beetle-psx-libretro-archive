/* No GPU context is created. These callbacks are leaf notifications only. */
void rhi_intf_set_tex_window(uint8_t w, uint8_t h, uint8_t x, uint8_t y)
{
   (void)w;
   (void)h;
   (void)x;
   (void)y;
}
void rhi_intf_invalidate_clut_cache(void)
{
}
void rhi_intf_toggle_display(bool off)
{
   (void)off;
}
void rhi_intf_set_draw_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
   (void)x0;
   (void)y0;
   (void)x1;
   (void)y1;
}
void rhi_intf_load_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *pixels,
                         bool mask, bool set_mask)
{
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   (void)pixels;
   (void)mask;
   (void)set_mask;
}
bool rhi_intf_read_vram(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *pixels)
{
   (void)x;
   (void)y;
   (void)w;
   (void)h;
   (void)pixels;
   return true;
}
void rhi_intf_set_vram_framebuffer_coords(uint32_t x, uint32_t y)
{
   (void)x;
   (void)y;
}
void rhi_intf_set_horizontal_display_range(uint16_t x, uint16_t y)
{
   (void)x;
   (void)y;
}
void rhi_intf_set_vertical_display_range(uint16_t x, uint16_t y)
{
   (void)x;
   (void)y;
}
void rhi_intf_set_display_mode(bool depth, bool pal, bool interlaced, int width)
{
   (void)depth;
   (void)pal;
   (void)interlaced;
   (void)width;
}
void PSX_EventVirtual(const int type, bool virt)
{
   (void)type;
   (void)virt;
}
void IRQ_Assert(int which, bool asserted)
{
   (void)which;
   (void)asserted;
}
void MDFN_FASTCALL TIMER_SetVBlank(bool blank)
{
   (void)blank;
}
