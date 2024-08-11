
int entry(int argc, char **argv) {	
	window.title = STR("nrkn dungeon");
	// We need to set the scaled size if we want to handle system scaling (DPI)
  window.scaled_width = 1280; 
	window.scaled_height = 720; 
	window.x = 100;
	window.y = 100;
	window.clear_color = COLOR_WHITE;

  Gfx_Image* player = load_image_from_disk(
    fixed_string("data/player.png"), get_heap_allocator() 
  );

  assert(player, "oh noes");
  
  Vector2 player_pos = v2(0, 0);
  Vector2 player_size = v2(6, 7);
  float player_speed = 50.0;

  float64 seconds_counter = 0.0;
  float64 last_time = os_get_current_time_in_seconds();
  s32 frame_count = 0;

  float zoom = 8;

	while (!window.should_close) {
		reset_temporary_storage();

    draw_frame.projection = m4_make_orthographic_projection(
      window.width * -0.5, window.width * 0.5, 
      window.height * -0.5, window.height * 0.5, 
      -1, 10
    );

    draw_frame.view = m4_make_scale(v3( 1.0 / zoom, 1.0 / zoom, 1.0));

    float64 now = os_get_current_time_in_seconds();
    float64 delta_t = now - last_time;
    last_time = now;

		os_update(); 

    if( is_key_just_pressed(KEY_ESCAPE) ){
      window.should_close = true;
    }

    Vector2 input_axis = v2(0, 0);

    if( is_key_down('A') ){
      input_axis.x -= 1.0;
    }
    if( is_key_down('D') ){
      input_axis.x += 1.0;
    }
    if( is_key_down('S') ){
      input_axis.y -= 1.0;
    }
    if( is_key_down('W') ){
      input_axis.y += 1.0;
    }
    input_axis = v2_normalize(input_axis);

    player_pos = v2_add(
      player_pos, v2_mulf(input_axis, player_speed * delta_t)
    );
		
		Matrix4 xform = m4_scalar(1.0);
    xform = m4_translate(xform, v3(player_size.x * -0.5, 0, 0));
    xform = m4_translate(
      m4_scalar(1.0), v3(player_pos.x, player_pos.y, 0)
    );

		draw_image_xform(
      player, xform, v2(player_size.x, player_size.y), COLOR_WHITE
    );	

		gfx_update();

    seconds_counter += delta_t;
    frame_count += 1;

    if( seconds_counter > 1.0){
      log( "fs: %i", frame_count );
      seconds_counter = 0.0;
      frame_count = 0;
    }
	}

	return 0;
}
