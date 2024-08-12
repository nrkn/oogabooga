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

  assert(entity_found, "no more entities!");

  return entity_found;
}

void entity_destroy( Entity* entity ){
  memset(entity, 0, sizeof(Entity));
}

void setup_rock( Entity* entity ){
  entity->arch = ARCH_rock;
  entity->sprite_id = SPRITE_rock0;
}

void setup_tree( Entity* entity ){
  entity->arch = ARCH_tree;
  entity->sprite_id = SPRITE_tree0;
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
  Sprite* sprite = get_sprite(entity->sprite_id);

  Matrix4 xform = m4_scalar(1.0);
  xform = m4_translate(xform, v3(sprite->img->width * -0.5, 0, 0));
  xform = m4_translate(
    m4_scalar(1.0), v3(entity->pos.x, entity->pos.y, 0)
  );
  draw_image_xform(
    sprite->img, xform, v2(sprite->img->width, sprite->img->height), COLOR_WHITE
  );	
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

  float zoom = 8.0;

  sprites[SPRITE_player] = (Sprite){ .img=load_data_image("data/player.png") };
  sprites[SPRITE_rock0] = (Sprite){ .img=load_data_image("data/rock-0.png") };
  sprites[SPRITE_rock1] = (Sprite){ .img=load_data_image("data/rock-1.png") };
  sprites[SPRITE_tree0] = (Sprite){ .img=load_data_image("data/tree-0.png") };
  sprites[SPRITE_tree1] = (Sprite){ .img=load_data_image("data/tree-1.png") };  

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
  }

  Entity* player_en = entity_create();  
  player_en->pos = v2(0, 0);
  player_en->arch = ARCH_player;
  player_en->sprite_id = SPRITE_player;
  player_en->speed = 50.0;

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
  }

  float64 seconds_counter = 0.0;
  float64 last_time = os_get_current_time_in_seconds();
  s32 frame_count = 0;

	while (!window.should_close) {
		reset_temporary_storage();

    draw_frame.projection = m4_make_orthographic_projection(
      window.scaled_width * -0.5, window.scaled_width * 0.5, 
      window.scaled_height * -0.5, window.scaled_height * 0.5, 
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

    player_en->pos = v2_add(
      player_en->pos, v2_mulf(input_axis, player_en->speed * delta_t)
    );
    
    for( int i = 0; i < MAX_ENTITY_COUNT; i++ ){
      Entity* en = &world->entities[i];

      if( !en->is_valid ){
        continue;
      }

      draw_entity(en);
    }

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
