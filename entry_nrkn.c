const int tile_width = 8;

int world_to_tile( float world ){
  return world / (float)tile_width;
}

float tile_to_world( int tile ){
  return (float)tile * (float)tile_width;
}

Vector2 round_v2_to_tile( Vector2 world_pos ){
  return v2(
    tile_to_world( world_to_tile( world_pos.x ) ),
    tile_to_world( world_to_tile( world_pos.y ) )
  );
}

bool almost_equals(float a, float b, float epsilon){
  return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target( float* value, float target, float delta_t, float rate ){
  *value += ( target - *value ) * ( 1.0 - pow( 2.0f, -rate * delta_t ) );

  if( almost_equals(*value, target, 0.001f ) ){
    *value = target;
    return true; // gottem
  }

  return false;
}

void animate_v2_to_target( Vector2* value, Vector2 target, float delta_t, float rate ){
  animate_f32_to_target(&(value->x), target.x, delta_t, rate);
  animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

typedef struct Sprite {
  Gfx_Image* img;
} Sprite;

typedef enum SpriteId {
  SPRITE_nil,
  SPRITE_rock0,
  SPRITE_rock1,
  SPRITE_tree0,
  SPRITE_tree1,
  SPRITE_player,
  SPRITE_max,
} SpriteId;

Sprite sprites[SPRITE_max];

typedef enum EntityArcheType {
  ARCH_nil = 0,
  ARCH_rock = 1,
  ARCH_tree = 2,
  ARCH_player = 3,
} EntityArcheType;

typedef struct Entity {
  bool is_valid;
  bool is_draw;
  EntityArcheType arch;
  Vector2 pos;
  float speed;
  SpriteId sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 1024

typedef struct World {
  Entity entities[MAX_ENTITY_COUNT];
} World;

World* world = 0;

Entity* entity_create(){
  Entity* entity_found = 0;

  for( int i = 0; i < MAX_ENTITY_COUNT; i++ ){
    Entity* existing_entity = &world->entities[i];

    if( !existing_entity->is_valid ){
      entity_found = existing_entity;
      entity_found->is_valid = true;
      break;
    }
  }

  assert(entity_found, "eee no ents");

  return entity_found;
}

void entity_destroy( Entity* entity ){
  memset(entity, 0, sizeof(Entity));
}

void setup_rock( Entity* entity ){
  entity->arch = ARCH_rock;
  entity->sprite_id = SPRITE_rock0;
  entity->is_draw = true;
}

void setup_tree( Entity* entity ){
  entity->arch = ARCH_tree;
  entity->sprite_id = SPRITE_tree0;
  entity->is_draw = true;
}

Gfx_Image* load_data_image( const char* path ){
  Gfx_Image* img = load_image_from_disk(
    fixed_string(path), get_heap_allocator() 
  );

  assert(img, "load_data_image: oh noes");

  return img;
}

Sprite* get_sprite( SpriteId id ){
  if( id >= 0 && id < SPRITE_max ){
    return &sprites[id];
  }

  return &sprites[0];
}

void draw_entity( Entity* entity ){
  if( !entity->is_draw ){
    return;
  }

  Sprite* sprite = get_sprite(entity->sprite_id);

  Matrix4 xform = m4_scalar(1.0);
  xform = m4_translate(xform, v3(sprite->img->width * -0.5, 0, 0));
  xform = m4_translate(
    m4_scalar(1.0), v3(entity->pos.x, entity->pos.y, 0)
  );
  
  draw_image_xform(
    sprite->img, xform, 
    v2(sprite->img->width, sprite->img->height), 
    COLOR_WHITE
  );	
}

Vector2 mouse_to_world() {
  float mouse_x = input_frame.mouse_x;
  float mouse_y = input_frame.mouse_y;
  Matrix4 proj = draw_frame.projection;
  Matrix4 view = draw_frame.view;
  float window_w = window.width;
  float window_h = window.height;

  // mormalize mouse coords
  float ndc_x = ( mouse_x / ( window_w * 0.5f ) ) - 1.0f;
  float ndc_y = ( mouse_y / ( window_h * 0.5f ) ) - 1.0f;

  // to world coords
  Vector4 world_pos = v4( ndc_x, ndc_y, 0, 1 );
  world_pos = m4_transform( m4_inverse( proj ), world_pos );
  world_pos = m4_transform( view, world_pos );

  return (Vector2){ world_pos.x, world_pos.y };
}

int entry(int argc, char **argv) {	
	window.title = STR("nrkn dungeon");
	// We need to set the scaled size if we want to handle system scaling (DPI)
  window.scaled_width = 1280; 
	window.scaled_height = 720; 
	window.x = 50;
	window.y = 250;
	window.clear_color = COLOR_WHITE;

  world = alloc( get_heap_allocator(), sizeof(World) );

  float zoom = 10.0;
  Vector2 camera_pos = v2(0, 0);

  sprites[SPRITE_player] = (Sprite){ .img=load_data_image("data/player.png") };
  sprites[SPRITE_rock0] = (Sprite){ .img=load_data_image("data/rock-0.png") };
  sprites[SPRITE_rock1] = (Sprite){ .img=load_data_image("data/rock-1.png") };
  sprites[SPRITE_tree0] = (Sprite){ .img=load_data_image("data/tree-0.png") };
  sprites[SPRITE_tree1] = (Sprite){ .img=load_data_image("data/tree-1.png") };  

  Gfx_Font *font = load_font_from_disk( 
    STR( "data/GothamRoundedMedium_21022.ttf"), get_heap_allocator()
  );
  assert(font, "bad times with font");

  const u32 font_height = 120;

  for( int i = 0; i < 10; i++ ){
    Entity* rock_en = entity_create();

    setup_rock(rock_en);

    if( i % 2 ){
      rock_en->sprite_id = SPRITE_rock1;
    }

    rock_en->pos = v2(
      get_random_float32_in_range(-100, 100),
      get_random_float32_in_range(-100, 100)
    );

    rock_en->pos = round_v2_to_tile(rock_en->pos);
  }

  Entity* player_en = entity_create();  
  player_en->pos = v2(0, 0);
  player_en->arch = ARCH_player;
  player_en->sprite_id = SPRITE_player;
  player_en->speed = 50.0;
  player_en->is_draw = true;

  for( int i = 0; i < 10; i++ ){
    Entity* tree_en = entity_create();

    setup_tree(tree_en);

    if( i % 2 ){
      tree_en->sprite_id = SPRITE_tree1;
    }

    tree_en->pos = v2(
      get_random_float32_in_range(-100, 100),
      get_random_float32_in_range(-100, 100)
    );

    tree_en->pos = round_v2_to_tile(tree_en->pos);
  }

  float64 seconds_counter = 0.0;
  float64 last_time = os_get_current_time_in_seconds();
  s32 frame_count = 0;

	while (!window.should_close) {
		reset_temporary_storage();
    
    float64 now = os_get_current_time_in_seconds();
    float64 delta_t = now - last_time;
    last_time = now;

    os_update(); 

    // camera
    {
      Vector2 target_pos = player_en->pos;
      
      animate_v2_to_target(&camera_pos, target_pos, delta_t, 15.0f);

      draw_frame.projection = m4_make_orthographic_projection(
        window.width * -0.5, window.width * 0.5, 
        window.height * -0.5, window.height * 0.5, 
        -1, 10
      );

      draw_frame.view = m4_make_scale(v3( 1.0, 1.0, 1.0));

      draw_frame.view = m4_mul(
        draw_frame.view, 
        m4_make_translation(v3(camera_pos.x, camera_pos.y, 0))
      );

      draw_frame.view = m4_mul(
        draw_frame.view, m4_make_scale(v3( 1.0 / zoom, 1.0 / zoom, 1.0))
      );
    }

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

    player_en->pos = v2_add(
      player_en->pos, v2_mulf(input_axis, player_en->speed * delta_t)
    );

    Vector2 mouse_in_world = mouse_to_world();
    int mouse_tile_x = world_to_tile(mouse_in_world.x);
    int mouse_tile_y = world_to_tile(mouse_in_world.y);

    int player_tile_x = world_to_tile(player_en->pos.x);
    int player_tile_y = world_to_tile(player_en->pos.y);
    int tile_radius_x = 40;
    int tile_radius_y = 30;
    int tile_left = player_tile_x - tile_radius_x;
    int tile_right = player_tile_x + tile_radius_x;
    int tile_top = player_tile_y - tile_radius_y;
    int tile_bottom = player_tile_y + tile_radius_y;

    for( int x = tile_left; x < tile_right; x++ ){
      for( int y = tile_top; y < tile_bottom; y++ ){     
        float x_pos = x * tile_width;
        float y_pos = y * tile_width;
        
        Vector4 col = hex_to_rgba(0xf0f0f0ff);

        if( (x + y) % 2 == 0 ){
          col = hex_to_rgba(0xfafafaff);
        }

        if( x == mouse_tile_x && y == mouse_tile_y ){
          col = hex_to_rgba(0x3399ffff);
        }

        draw_rect( v2(x_pos, y_pos), v2(tile_width, tile_width), col );        
      }
    }

    for( int i = 0; i < MAX_ENTITY_COUNT; i++ ){
      Entity* en = &world->entities[i];

      if( !en->is_valid ){
        continue;
      }

      Sprite* sprite = get_sprite(en->sprite_id);
      Vector2 sprite_size = v2(sprite->img->width, sprite->img->height);
      Vector2 sprite_pos = v2_add(en->pos, v2(sprite_size.x * 0.5, 0));
      Range2f bounds = range2f_make_bottom_center(sprite_size);
      bounds = range2f_shift(bounds, sprite_pos);
      
      Vector4 col = hex_to_rgba(0x3399ffff);

      col.a = 0;

      if( range2f_contains(bounds, mouse_in_world) ){
        col.a = 0.75;
      }
      
      draw_rect(bounds.min, range2f_size( bounds ), col );
    }

    for( int i = 0; i < MAX_ENTITY_COUNT; i++ ){
      Entity* en = &world->entities[i];

      if( !en->is_valid ){
        continue;
      }

      draw_entity(en);
    }

    // draw_text( 
    //   font, 
    //   sprint( temp_allocator, STR("%f %f"), mouse_in_world.x, mouse_in_world.y), 
    //   font_height, mouse_in_world, v2( 0.01f, 0.01f), COLOR_BLUE
    // );

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
