# BineCrabric Lua 5.4 mod agent

Use this document as model context; append the requested mod afterward. Implement documented APIs only. Source snapshot: 2026-10-25; Lua LOD/concurrent additions excluded.

## Instructions and notation

Deliver complete files under mods/<id>/, manifest and assets included. Local variables, small functions, bounded work, no source comments/placeholders. Preserve unrelated edits. No Git inspection/publication, engine edits, or competing builds unless requested. Check sides, returns, coordinates, item conservation, failure paths. Report actual validation; syntax/stubs do not establish gameplay/network correctness.

Signatures omit `minecraft.` except named Lua libraries. `?`=optional; `=value`=default; `->`=returns. Untagged code blocks are notation, not executable Lua. x/y/z means three separate fields. Functions use dot calls; entity handles,event.chunk,event.ui use colon methods. Lua arrays 1-based; inventory slots 0-based.

## Package/runtime

Root mod.json, scripts/main.lua; optional config/init.lua,helpers,models,textures,sounds. ZIP root must contain mod.json, not an enclosing folder.
```json
{"id":"my_mod","name":"My Mod","version":"1.0.0","enabled":true,"side":"client","entry":"scripts/main.lua"}
```
Manifest id required: letters/digits/`.`/`_`/`-`; prefer lowercase underscores (dots conflict with settings). name defaults id,enabled=true,side=both(client/server/both). Optional version,description,download_url. entry relative .lua; legacy script fallback. Empty entry=resource-only. No dependency/load-order solver.

Install directly in <run>/mods/. Windows default run: %APPDATA%/.minecraft,then %USERPROFILE%/.minecraft,then cwd/.minecraft. Repo mods/ need not be live. Enabled overrides mods/mod-state.cfg; ZIP cache mods/.cache/. Limits archive256MiB,entry64MiB,extracted512MiB,4096 entries; unsafe paths rejected.

Separate Lua state/globals/require cache per package. require("config")->config/init.lua; require("scripts.helper")->scripts/helper.lua; require("lib.settings")->mods/lib/settings/init.lua: install shared mods/lib beside packages. No shared cross-mod Lua table. Search package-root ?.lua,?/init.lua,mods-root equivalents,then existing package.path.

Entry once,callbacks synchronous; errors log,later listeners may continue. Logs [lua-mod:id:level]. Mods screen toggles/rescans. Unload clears callbacks/owned textures/instances; static content/recipes not transactional. Same-owner registration may keep old definition. Restart after changing IDs/models/recipes; never recycle saved IDs. Required content tracking separate.

No io,debug,dofile,loadfile,native modules/process execution. os only clock/date/difftime/time; load/require text-only. Normal math/string/table/utf8/coroutine remain; no automatic coroutine scheduling. No HTTP/thread/timer/general engine-object API. Not a strong filesystem sandbox; trust mods. No script time/memory quota.

## Critical rules

- Client-only model,gui,screen,inventory,texture,sound,render,files,session,options,raycast,items.describe. Both-side mods guard initialization, e.g. if minecraft.model then ... end.
- Manifest client=executable side. Singleplayer world usually remote=false. is_client()=context,not write permission. Check setters' returns.
- Handles are snapshots; handle.x assignment does nothing. Refresh after ticks/world changes; player() may be nil.
- Chunks16x128x16; integer Y0..127,local X/Z0..15. Negative coordinates need floor division. block_id failure0 is truthy in Lua: test >0.
- Check writes before consuming items. set_block has no drops/tool wear. give drops overflow; offer returns remainder: never discard. Stack APIs omit arbitrary NBT.
- Never retain event.chunk/ui outside callback. Cache data; no disk/model/texture/probe work per frame. Filter render stage/moment/shadow.
- GUI/pixels ARGB integers0xAARRGGBB; render channels generally0..1. Transforms degrees except explicit radians; celestial phases turns. Never subtract camera twice.

## Events/subscription

on(event,options,callback)->bool; always options({} allowed); unknown event logs/false. Return event after mutation; nil/false returns original; another table replaces it. Returning true does not cancel. Only documented writable fields reach native state. Each listener gets a fresh table with prior native readback; extra Lua keys do not propagate. Errors do not commit event readback.

Options priority=0(higher first),once=false(inactive after successful match; not reentrancy-safe),when(event)(must return exactly true). Other keys filter event fields by scalar equality,accepted array,true-valued set,or predicate(actual). Missing fields nil. No unsubscribe/automatic stop-propagation; respect canceled/handled.

at_phase(name,order,callback): init/post_init/ready; smaller order first; callback numeric previous/current. Register at entry. Current bookkeeping may suppress multiple callbacks per mod/phase: use one and call helpers. No assumed immediate late replay.

## Core/input/utilities

```
mod_id                                  manifest ID
log(message) | log(level,message)        info/warn/error
notify(text)->bool                      client chat; false without client
is_client()->bool                       context
is_key_down(code)->bool                 false on server
is_mouse_down(button)->bool             left0,right1; false on server
key_code(name_or_number)->integer       unknown0; case-sensitive
time.utc_millis()->number               Unix milliseconds
options.get(key)->bool/number/string/nil client
options.keys()->string[]                persistent option keys
options.cycle(key,delta=1)               client; no options.set
drop_item(id,count,x,y,z)->bool          client world; damage0,pickupDelay10,no NBT
util.clamp(value,min,max)->number
util.in_rect(x,y,left,top,width,height)->bool left/top inclusive,right/bottom exclusive
util.real_world(event)->bool             event exists and mod_generation~=false; not proof of world
util.parse_boolean(value)->bool/nil      trimmed/case-insensitive true/1/yes/on,false/0/no/off; bool/numeric accepted
util.resolve_seed(text)->integer         retain integer for exact64-bit identity
util.json_encode(table)->string | nil,error root table,no cycles/functions/userdata
util.json_decode(text)->value | nil,error
util.json_null                          null sentinel,not nil; preserves array slots
registry.name(domain,id)->string        biome shared,block client; unknown="unknown"
registry.list(domain)->string[]         unsupported empty
```
key_code binding names forward,left,back,right,jump,sneak,drop,inventory,chat,fog; literals escape,enter,space,0..9,a..z,up,down,left_arrow,right_arrow. Legacy codes,not GLFW/OS. options.get also skin,lastServer,key bindings.

## Assets/storage

```
asset_path(relative)->string/nil         existing package file/dir; "." root
list_assets(directory)->string[]         sorted immediate file basenames
read_asset(relative)->string/nil         text
read_asset_bytes(relative,{gzip=false}?)->string/nil empty/missing/bad gzip=nil
read_nbt_asset(relative)->value,nil | nil,error plain/gzip,entry-size limit
storage.read(relative)->string/nil
storage.write(relative,text)->bool
files.pick({extension="json"})->string/nil client dialog; nil cancel
files.read(path)->string | nil,error     client resource then filesystem; empty fails
```
Assets package-root relative. Storage safe relative under <run>/config/mods/<sanitized_id>/; no delete/list/per-world isolation. Check write before cache update. files.read strips leading resources/ for resource lookup.

Resource precedence: active packages reverse display-name sort; per package resources/path for overlays,root/path,root/assets/path. Namespace global resource paths. Models load explicit package-root path. NBT reads compounds->keyed tables,lists/int/long arrays->arrays,byte arrays->binary string,numerics->numbers. Writeback limited to small string-keyed compounds/scalars,signed32-bit integers; no typed list/64-bit/cycle guarantee.

Per-world creation options: create_world.options string map,restore world_open.options. Max256 entries,nonempty keys<=128 bytes,values<=4096. Not arbitrary per-tick storage.

## Settings/keybinds

settings=require("lib.settings"); settings.define(mod_id,definition)->config,cached once per ID/state. Use minecraft.mod_id. Read/assign config.field; unknown keys ordinary properties. definition name,native_ui=true,hide_with_shader_pack=false,fields={field=spec}; no guaranteed row order.

spec type,key(default field name),label,default,min,max,step,decimals,integer,options:
- omitted/slider/number clamp min0,max1; invalid/NaN->min; integer=true rounds.
- int/integer clamp then floor,min0,max2147483647,native step=1/decimals=0. Prefer explicit bounds.
- bool/toggle true only for true,1,"true"; else false.
- options/cycle/enum string; options array; listed default.
- string text storage,custom UI.
native_ui=false stores all fields settings/<key>.txt; strings always storage. Read once; failed write raises,preserves cache. hide_with_shader_pack hides group,not callbacks.
```
settings.register(display_name,entries,options?) options.hide_with_shader_pack; no return
settings.get("mod_id.key")->bool/number/string/nil
settings.set("mod_id.key",value)                 saves,no return
keybinds.register(local_id,{default=code,label=text})
keybinds.get_code(full_id)->integer              full ID required; unknown0
keybinds.consume(local_or_full_id)->bool         queued press once; poll client_tick
```
Low-level entry key,label,kind: toggle default bool; slider min/max/step/decimals/integer/default; options/cycle/enum options strings + string/zero-based default. get option string(invalid index may number). set toggle bool,slider numeric(no assured clamp/quantization),options string/zero-based index. Prefer proxy. Native file <run>/mod_settings.txt; keybind ID mod_id.local_id.

## World/generation

Shared API,active event world/player with client fallback where implemented; menus/probes may lack world. Authoritative=nonremote.
```
world.block_id(name)->integer                   unresolved0
world.get_block(x,y,z)->integer                 air/invalid/unavailable0
world.get_block_meta(x,y,z)->integer            no world0
world.set_block(x,y,z,id,meta=0)->bool           authoritative,Y0..127,ID0..255,meta&15
world.harvest_block(x,y,z,required_item_id=0)->bool authoritative+active player
world.random(bound=1000)->integer               [0,bound),positive bound; no world0
world.is_night()->bool                          client published celestial state
world.get_time()->integer                      absolute ticks,no world0; day24000
world.set_time(ticks)->bool                     reject negative; remote changes local clock
world.get_top_y(x,z)->integer                   native top-solid query,no world-1
world.get_heightmap(origin_x,origin_z,width=128,height=128)->integer[]/nil
world.player()->handle/nil                      adds name
world.spawn_entity(type,{x,y,z})->bool           authoritative,table defaults0,64,0
world.spawn_entity(type,x,y?,z?)->bool           table also accepts triple
world.count_entities(type)->integer
world.get_difficulty()->integer                 0 peaceful,1 easy,2 normal,3 hard; no world0
world.set_difficulty(0..3)->bool                 authoritative
world.get_weather()->string                    clear/rain/thunder; no world clear
world.set_weather(mode)->bool                   authoritative
world.get_biome(x,z)->string/nil                 biome wire name
world.dimensions()->{{id,name,nether,has_ceiling,has_ground,terrain_styles}}  registered dimensions; terrain_styles=true when the dimension honors option terrain_style:<id>=beta|alpha|modern
world.set_spawn(x,y,z)->bool                    authoritative,Y clamped0..127
world.get_spawn()->{x,y,z}/nil
world.get_block_collisions(box)->box[]/nil       box min_x/y/z,max_x/y/z
world.sample_channels()->string[]
world.sample(seed,center_x,center_z,options?)->result | nil,error
particles.spawn(spec)->bool                    client particle,else false
items.ids()->integer[]                         instantiated item IDs
```
harvest_block checks positive required held ID,removes block,break effects,tool wear,harvestable drops; stops if tool breaks/disappears. set_block no mining effects. set_time authoritative sync updates; celestial hook for display-only sun. Weather clear/sun/sunny;rain/storm;thunder/thunderstorm;active timers12000.

get_heightmap dimensions1..512,loaded chunks only,row-major ARGB: low byte opaque topY+1,green translucent opacity,red translucent topY+1,alpha255; unloaded zero data bytes. Distinct get_top_y. Particle defaults x=0,y=64,z=0,vx/vy/vz=0,scale=4,r/g/b=1,max_age=40 ticks,gravity=.04; no type argument.

sample synchronous offline,not live data; seed currently signed int,not full64. Center clamps+/-30000000. options radius_chunks=6(1..4096),max_side=48(8..256),channel="grass" or channels array(max8 before dedup),mod_generation=false. Channels height,surface_block,surface_block_below,biome_id,grass. Result side,step,origin_x/z,center_x/z,channel(first),channels={name=values}; no values field. Index=row*side+column+1 for zero-based row/column,coords=origin+(column,row)*step. Surface height-1,below height-2,grass opaque ARGB. Custom generation opt-in; cache small probes.

Temporary chunk methods,callback only:
```
event.chunk:get_block(local_x,y,local_z)->integer invalid0
event.chunk:set_block(local_x,y,local_z,id)->bool/nil
event.chunk:fill(x1,y1,z1,x2,y2,z2,id)->integer inclusive,swaps reversed,clamps,counts successful writes
event.chunk:get_height(local_x,local_z)->integer firstY above light-blocking cell; invalid/empty0
```
Validate IDs0..255; no metadata. terrain/surface/carver raw writes;features chunk API. World origin=chunk_x/z*16. Never retain chunk.

## Entities/raycast

Handle snapshot id,type,x/y/z,vx/vy/vz,yaw,pitch,on_ground,sneaking,fire_ticks,is_on_fire. Living adds health,jumping;player held_item_id;mod registry_id,client_local,data;item item_id,item_count,item_damage,item_max_damage + client texture fields.
```
entities.list(type?)->handle[]                  exact built-in/mod type
entities.get(id)->handle/nil
entities.set_state(handle,values)->bool         handle:set_state(values)
entities.teleport(handle,{x,y,z,yaw,pitch})->bool handle:teleport(spec)
entities.teleport(handle,x,y,z,yaw?,pitch?)->bool
entities.remove(handle)->bool                   handle:remove(); refuses players
entities.spawn_mod("mod_id:name",spec)->handle/nil
entities.register_global_pose_hook(entity_type,callback)->bool
entities.register_local_pose_hook(entity_id,callback)->bool
entities.unregister_local_pose_hook(entity_id)->bool
raycast(options?)->hit/nil                       client
```
set_state x/y/z(must include numeric x),vx/vy/vz,yaw,pitch,fire_ticks,fall_distance,no_clip,health(living0..20),data(mod only); not on_ground/jumping/inventory. Remote mutations only owned client-local mod entities; authoritative targets others except player removal.

spawn_mod namespace=current mod_id; spec x/y/z/yaw/pitch=0,client_local=false,data. Remote needs client_local=true. Generic entity,not class declaration. Pose registration needs loaded owner: use live callback/lifecycle,or on("entity_render") at entry. Callback entity_id,entity_type,tick_delta,pose; return event. Local hooks cleared on removal.

raycast requires client world/camera even explicit. options origin named/triple or origin_x/y/z;direction named/triple or yaw/pitch degrees;max_distance/reach;ignore_liquids=false,blocks=true,entities=true. Normalized direction;absent/zero uses camera pose/reach(default5). Hit common type,hit_x/y/z. block block_x/y/z,side,block_id,block_name,item_id(=block). entity entity_id,entity_type,entity_raw_id,entity_x/y/z. model model_id,model_tag,distance. Limitation: no consistent global nearest selection across block/entity/model.

## Inventory (client)

Stack={id,count,damage};empty={id=0,count=0,damage=0}. Nonempty reads add max_damage,damageable,stackable,has_subtypes,max_count. Setters omit NBT. Low-level local operations,no multiplayer container sync.
```
inventory.slot_count()->integer                 40
inventory.main_size()->integer                  36
inventory.get(slot)->Stack/nil                   main0..35,armor36..39
inventory.set(slot,Stack)->bool                  replace; caller validates
inventory.cursor_get()->Stack/nil
inventory.cursor_set(Stack)->bool
inventory.give(Stack)->bool                      inserts+drops overflow; true!=room
inventory.offer(Stack)->Stack                    remainder; no player original
inventory.selected_slot()->integer              hotbar; no player0
items.describe(id)->table/nil                    id,max_damage,damageable,stackable,has_subtypes,max_count
```
Validate before changing inputs. Return owned stacks/cursor on close; account for old cursor before opening. Preserve counts on failure. repair_table inventory_helper is private,not minecraft.screen.slots.

## Content

register_block(spec),register_item(spec),register_shaped_recipe(spec)->true or raise. At entry before content phases. Unoccupied numeric blocks1..255,items256..31999. Never change saved IDs; same-owner registration can retain old spec.

Block defaults:
```
id required; model baked handle required on client,omitted on server
hardness=1,resistance=1,luminance=0 (0..1)
translation_key?,name?,material="stone" (stone/metal/wood/glass; unknown stone)
opaque=true,full_cube=true,translucent=not opaque
collision_height=1,stack_on_same=false,requires_solid_below=true
coordinate_bounds=false,coordinate_color=false
bounds_padding=.0625,bounds_offset=.1,min_scale=.9,max_scale=1.1
item_texture?,on_use?,behavior_priority=0
```
Baked bounds affect fullcube/collision;collision_height>1 extends collision. on_use installs block_interact block_id=id,right_click=true. Language tile.<translation_key>.name. No draw-callback models.

Item id,texture required(even with model),max_count=64,max_damage=0,translation_key?,name?,numeric model?. Language item.<translation_key>.name. Basic item,no arbitrary class/on-use fields; use events.

Recipe exactly one output_block_id/output_item_id,output_count=1(1..64),rectangular pattern1..3 rows/cols;key="#",item_id or ingredients={A=id,B=id}. Nonempty ingredients replaces single pair. One-byte symbols,spaces empty; valid item for each symbol. Post-init instantiation,no shapeless/smelting API,no full reversible reload/dedup guarantee.

## Models/assets (client)

model.load("models/my_mod/block.json") reads package-root path; assert result. JSON root parent?,texture_size[16,16],textures={name:path},elements. Element from/to triples0..16,faces,shade=true,rotation? {origin,axis=x/y/z,angle,rescale} or Euler x/y/z. Face keys down/up/north/south/west/east: texture,uv[u0,v0,u1,v1]?,rotation multiple90,tintindex?,cullface?. Only declared faces exist;#name texture reference,cycles/unresolved fail. Texture my_mod:block ->assets/my_mod/textures/block.png;explicit assets/.../mods/... preserved;relative beside declaring model +.png. Parent namespace ->assets/<namespace>/models/<name>.json; textures merge,elements inherit if omitted. Modern schema subset,no assumed blockstates/display overrides/animations. Baked geometry block units,JSON16 units.
```
model.load(package_path)->handle | nil,error
model.build({key?,quads})->handle | nil,error
model.place(handle,options?)->instance_id | nil,error
model.update(instance_id,options)->bool          replaces transform,omitted resets
model.remove(instance_id)->bool
model.clear()                                   this mod's instances
model.bounds(handle)->box/nil
model.draw(handle,options?)->bool                active world draw context
model.draw_item(item_id,damage=0,options?)->bool  numeric damage before options
model.item_bounds(item_id,damage=0)->box/nil
```
box min_x/y/z,max_x/y/z. Transform x/y/z=0,yaw/pitch/roll=0 degrees,scale=1,pivot_y=0;place adds tag. Immediate options brightness=-1(auto,explicit0..1),a=1,blend=false,cull=true,depth_test=true,depth_write=true,layer,entity_id=0,shader_entity="",block_id=0,block_meta=0. World coordinates,no camera subtraction.

build quads texture,shade/r/g/b/a=1,vertices four{x,y,z,u,v} in block units. Requires usable quad. Nonempty key global cache: namespace with mod_id;existing returns old model. Cache handle.

## Textures/sound/render (client)

```
texture.size(path)->{width,height}              missing zero dimensions
texture.pixel(path,x,y)->{a,r,g,b} | nil,error   zero-based,channels0..255
texture.bind(id,unit=0)->bool | nil,error        custom/rawGL; restores active unit
texture.create({width,height,pixels})->{id,width,height} | nil,error
texture.update(owned_id,{width,height,pixels})->bool | nil,error
texture.release(owned_id)->bool
texture.get_pixels(path_or_id)->{width,height,pixels}/nil
sound.register(id,path,kind="effect")->bool | false,error | nil,error
sound.play(id,volume=1,pitch=1)->bool
sound.play_at(id,x=0,y=0,z=0,volume=1,pitch=1)->bool
sound.play_loop_at(id,x=0,y=0,z=0,volume=1,pitch=1)->string_handle/nil
sound.stop(handle)->bool
render.quads(spec)->emitted_quad_count
render.billboards(spec)->emitted_count
render.set_item_entity_override(bool)            global,coordinate ownership/restore
```
Pixels dense row-major ARGB integers;positive bounded dimensions/full array. update/release owned only;unload frees. create ID custom registry;gui.texture_id rawGL:not interchangeable everywhere. get_pixels needs client. Sound kind effect/streaming/music;existing resource,namespace IDs,stop loops.

quads active world context(world_render): vertices named{x,y,z,u,v,r,g,b,a} array,multiple4;or packed stride9 x,y,z,u,v,r,g,b,a. Max65536 vertices;incomplete trailing quad dropped. Defaults r/g/b/a=1,blend=false,cull/depth_test/depth_write=true. texture path or texture_id;layer,entity_id,shader_entity;transform x/y/z,yaw/pitch/roll,scale;world_space=false. true adjusts origin by camera eye;false current frame. Packed colors do not inherit defaults.

billboards sky-direction sprites,not world entity sprites: billboards{x,y,z,size=.2,alpha=1} array or packed stride5 x,y,z,size,alpha. brightness=1,rotation_x_rad=0,rotation_y_rad=0,blend="alpha"/"additive",depth_test/depth_write=false,layer. Max65536;cache arrays. Layers entity,terrain,block,sky,clouds,basic,textured;empty/unknown auto. Not event stages. No rawGL/shader/FBO/LOD API documented.

## Screens/GUI/session (client)

screen.open(id,{title="",pause=true}?);register screen_event first. screen.close() parent/game. GUI draw only screen/region render,not ticks.
```
screen.add_field(name,x,y,width,height=20,options?) init only
screen.field_text(name)->string                    absent=""
screen.set_field_text(name,text)
screen.set_fields_visible(bool)
screen.add_button(x,y,width,height,text,on_click?) init only,native input
screen.host_field(name)->string
screen.host_set_field(name,text)
screen.open_host(screen_id,string_fields?)         supported factory only
gui.fill_rect(x,y,width,height,argb)
gui.draw_text(x,y,text,argb)
gui.draw_centered_text(x,y,width,text,argb=0xFFFFFFFF)
gui.draw_centered_text({x,y,width,text,color})       width alias w,white default
gui.draw_item(x,y,item_id,count,damage=0)
gui.text_width(text)->integer
gui.texture_id(path)->integer                       rawGL,unavailable0
gui.draw_sprite(path,x,y,u,v,width,height)          legacy atlas pixel UV
gui.draw_texture(gl_id,x,y,width,height)            whole texture
gui.draw_button(spec)
gui.draw_slider(spec)
gui.draw_toggle(spec)
event.ui:add_centered_button(y,text,on_click?,label_callback?)
event.ui:add_button(x,y,width,height,text,on_click?)
event.ui:add_stacked_centered_button(text,on_click?,label_callback?)
```
Field options text,max_len=0,numeric/signed/decimal=false. Widget spec x,y,width,height,hovered? or mouse_x/y;button text,active=true;slider text,value0..1;toggle label,value bool. Draw helpers no input. label_callback updated text after click. event.ui only during screen_ui. Coordinates scaled GUI,not framebuffer.

screen.ids: login,title,game_menu,multiplayer,connect,disconnected,downloading_terrain,death,chat,sleeping_chat,confirm,create_world,select_world,edit_world,world_settings,world_save_conflict,inventory,crafting,dispenser,double_chest,furnace,sign_edit,options,video_options,detail_settings,keybinds,mods,mod_settings,achievements,stats,lan,lan_info,server_mod_download,fatal_error,out_of_memory. Use constants,not guessed strings/factories. regions footer,screen,side_panel;host determines emissions.

session.set_offline_username(text),clear_offline_username();is_offline_mode()/is_authenticated()->bool;get_offline_username()/get_username()->string. Local selection,no account-login/server-auth API.

## Lua libraries

require("lib.commands") separate registry/hook/help per state. Remote slash commands pass through;/help allowed despite cancellation,multiple registries may print.
```
commands.register({name,run,params?,help?,details?,category?,aliases?})->true
commands.unregister(name)->bool
commands.list()->entry[]
commands.execute(message,event?)->bool    claimed,including errors; remote=true passes through
commands.install()                       idempotent,register calls it
commands.reply(text)
commands.number(text,label?)->number     asserts invalid; caller bounds/nonfinite check
commands.integer(text,label?)->integer   floor
commands.help_title                     default category
```
Context name,args(1-based whitespace tokens),raw,rest,event,entry,reply(text),usage(). No quoted parser;errors displayed. /help[page|command],/? automatic,8 lines/page incl headings. Name "/set" invoked //set;unknown passes through. execute itself does not cancel;installed hook does. Avoid alias collisions. Colors GRAY,RED,YELLOW,PURPLE,WHITE,AQUA.

require("lib.mathx"):tau,deg_to_rad;clamp(v,lo,hi),mix(a,b,t),smoothstep(edge0,edge1,x),length3(x,y,z),normalize3(x,y,z)->x,y,z(zero fallback0,1,0),dot(ax,ay,az,bx,by,bz),cross(same)->x,y,z,hash01(a?,b?,c?,d?)->[0,1),randomUnit(seed,channel)->x,y,z. Visual float hash,not native Java RNG.

## All 37 events

Groups E=side,remote,shader_pack_active,shader_pack_name. W=has_world,world_name,is_overworld,is_nether,world_seed,mod_generation. I=entity_id,entity_type. H=item_id,item_count,item_damage(when has_item). T=item_id,item_count,item_damage,item_max_damage,texture_path,mod_texture,atlas_index(item entities only,no position/age). R=readable;write=only native readback. Unlisted groups not guaranteed. Worldless W false/empty/0;generation overrides seed/mod_generation/is_overworld. E side=context,not manifest.

- `client_tick` R:E+W,before,after_world,paused,has_player,camera_y,player_y,player_fall_distance,player_on_ground,world_time(mod24000),is_day;write:none. Filter phase.
- `render_frame` R:E,tick_delta;write:none.
- `first_person_hand` R:E,tick_delta,eye,canceled,camera I if present;write:canceled.
- `key_press` R:E,key,pressed,repeat,handled;write:handled.
- `mouse_button` R:E,button,pressed,handled;write:handled.
- `raycast` R:E,has_hit,type(none/block/entity),hit_x/y/z,block_x/y/z,side,block_id,block_name,item_id;entity hit I,entity_raw_id,entity_x/y/z;write:none. Observation,not raycast().
- `fov` R:E,tick_delta,fov;write:fov degrees.
- `camera_setup` R:E,tick_delta,x/y/z,yaw/pitch/roll,custom_view,hide_first_person_hand when frame exists;write:frame fields except tick_delta. World positions,degrees.
- `player_travel` R:E,sideways,forward,speed_multiplier,flying,has_player,is_local_player;write:sideways,forward,speed_multiplier,flying.
- `tick_rate` R:E,target_tps,tps_scale;write:both,finite positive.
- `world_start` R:E,save_name,new_world;write:none.
- `world_open` R:E,save_name,new_world,options(string map);write:none. Restore saved choices.
- `world_tick` R:E,remote,before;write:none. Authoritative filter remote=false,before=false;no W guaranteed.
- `create_world` R:E,save_name,seed,canceled,options(string map);write:canceled,options. Namespace keys.
- `world_spawn_search` R:W,x/y/z,resolved;write:x/y/z,resolved;authoritative.
- `chat_send` R:E+W,message,canceled;write:message,canceled.
- `chat_receive` R:E+W,message,canceled;write:message,canceled.
- `block_interact` R:E,x/y/z,block_id,side,right_click,remote,canceled,handled,has_player,local_player,has_item;player player_x/y/z,player_yaw/pitch;item H,item_max_damage,item_damageable. write:canceled,handled,item_count>=0,item_damage if damageable(clamp0..max).
- `block_attack` R:E,x/y/z,side,block_id,block_meta,initial,canceled,handled,remote,has_player,local_player,has_item,H;write:canceled,handled.
- `block_break` R:E,x/y/z,original block_id/block_meta,remote,has_player,local_player,has_item,H,item_max_damage,item_damageable,item_is_axe;write:none. After successful mining,cannot veto.
- `entity_interact` R:E,attack,remote,canceled,handled,sneaking,has_player,local_player,has_target,has_item,player_yaw/pitch,H,target I,target_id when present;write:canceled,handled,item_count.
- `attack_damage` R:E,damage,critical,canceled,fall_distance,on_ground,target_x/y/z,has_player,has_target;write:damage,critical,canceled. No target ID.
- `entity_tick` R:E,remote,canceled,I,x/y/z,yaw,pitch if entity;write:canceled.
- `entity_teleport` R:W,I,from_x/y/z,x/y/z,yaw,pitch,canceled,has_entity,has_player;write:destination,yaw,pitch,canceled. Client-build dispatcher only,skips remote entities.
- `entity_spawn` R:I+T if item;write:none;client dispatcher.
- `entity_remove` R:I+T if item;write:none;client notification,local pose cleanup regardless.
- `celestial_state` R:E+W,tick_delta,celestial_angle,sun_angle,shadow_angle,moon_phase,sun_x/y/z,moon_x/y/z,is_day,pole_altitude_deg,sidereal_angle,override_directions,override_star_frame,override_phase,clock_angle. write:listed except E/W/tick_delta/clock_angle;phase_owner string additionally writable. Phases turns,pole degrees,world directions. Claim override_phase=true+phase_owner;direction/star override needs matching flags/data. Don't change simulation clock per frame.
- `world_color` R:E+W,partial_ticks,r/g/b,kind(sky/fog);client world celestial_angle,sun_angle,shadow_angle,is_day,world_time(mod24000);write:r/g/b finite clamp0..1.
- `fog_settings` R:E+W,enabled,spherical,exponential,start,end,density,custom_color,red/green/blue;write:listed settings,numeric clamp0..1. start/end normalized,not distance;custom_color=true for RGB.
- `entity_render` R:E,I,is_player,tick_delta,pose;write:pose. Pose body_yaw,head_yaw,head_pitch,limb_swing,limb_distance,yaw,pitch,roll,scale,offset_x/y/z,parts={name={yaw,pitch,roll}}. Part names/angles native-model-specific;preserve others.
- `pre_entity_render` R:I,tick_delta,canceled,T if item;write:canceled;client.
- `world_render` R:stage,moment,tick_delta,shadow_pass,stage_enabled,cancel_vanilla,vanilla_stage_ran,W,world_time,is_day,star_brightness,rain_strength,cloud_base_height,render_distance,cloud_r/g/b,astronomy_enabled,astronomy_utc_millis,observer_latitude_deg,observer_longitude_deg,celestial_angle,sun_x/y/z,moon_x/y/z,camera_x/y/z,eye_x/y/z,camera_yaw/pitch/roll,custom_camera,shader_pack_active,shader_pack_name. write:cancel_vanilla,star_brightness,rain_strength,astronomy_enabled,astronomy_utc_millis,observer_latitude_deg,observer_longitude_deg. Brightness/rain clamp0..1. Client3D scope;filter stage/moment/shadow;cancel before vanilla. Eye geometry origin may differ from pose.
- `chunk_generation` R:stage,moment,cancel_vanilla,vanilla_stage_ran,W,world_seed,mod_generation,is_overworld,dimension_id,chunk_x/z,has_chunk,chunk;write:cancel_vanilla;chunk methods mutate. Stages terrain/surface/carver/features,moments before/after. Authoritative incl probes without world. Cancel before,supply full replacement. Deterministic seed/coords,not clock.
- `snow_ice_placement` R:E,x/y/z,place_snow,place_ice;write:place_snow,place_ice.
- `screen_ui` R:screen_id,region,host_fields,ui;write:none;ui colon methods during callback.
- `screen_region` R:phase_name(render/mouse_click/mouse_scroll),screen_id,region,mouse_x/y,button,scroll_delta,x/y,width,height,handled;write:handled,width,height. Draw scope render only.
- `screen_event` R:screen_id,phase(init/render/tick/key/mouse/scroll/close),width,height,mouse_x/y and x/y aliases,tick_delta,key,char(unsigned byte),button,released,delta,handled;write:handled. Controls init,draw render,return items close;ignore released clicks.

world_render stages sky,stars,terrain_opaque,entities,particles_lit,particles,terrain_translucent,weather,clouds,hand,framebuffer;moments before/after. Shadows may repeat. stage_enabled/cloud/camera/render_distance read-only. screen_region phase_name vs screen_event phase. Reset world-specific IDs/queues between worlds;state survives transitions. Lifecycle names don't guarantee resources ready.

## Working examples

Client position display: use manifest above; scripts/main.lua:
```lua
local config = require("lib.settings").define(minecraft.mod_id, {
  name = "My Mod", fields = {enabled = {type = "bool", default = true}},
})
minecraft.keybinds.register("position", {default = minecraft.key_code("p"), label = "Show position"})
minecraft.on("client_tick", {before = true}, function(event)
  if not minecraft.keybinds.consume("position") then return event end
  if not config.enabled or not event.has_player or event.paused then return event end
  local player = minecraft.world.player()
  if player then minecraft.notify(string.format("%.1f, %.1f, %.1f", player.x, player.y, player.z)) end
  return event
end)
```
Both-side block: manifest side="both";check ID190 unoccupied. scripts/main.lua:
```lua
local model
if minecraft.model then model = assert(minecraft.model.load("models/my_mod/cube.json")) end
minecraft.register_block({id = 190, name = "Example Block", translation_key = "myModBlock",
  material = "stone", hardness = 2, resistance = 10, stack_on_same = true, model = model})
minecraft.register_shaped_recipe({output_block_id = 190, output_count = 4,
  pattern = {"##", "##"}, key = "#", item_id = 1})
```
models/my_mod/cube.json:
```json
{"texture_size":[16,16],"textures":{"all":"cube"},"elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"down":{"texture":"#all","uv":[0,0,16,16]},"up":{"texture":"#all","uv":[0,0,16,16]},"north":{"texture":"#all","uv":[0,0,16,16]},"south":{"texture":"#all","uv":[0,0,16,16]},"west":{"texture":"#all","uv":[0,0,16,16]},"east":{"texture":"#all","uv":[0,0,16,16]}}}]}
```
Provide models/my_mod/cube.png(16x16);copy bundled mods/stone_bricks/models/stone_bricks/stone_bricks.png for starter. No runtime dependency after copying;never reference missing assets.

## Verification/source lookup

Run python tools/check-lua-mods.py: parses mod scripts/document Lua+JSON,checks all event names,runs Lua5.4 behavioral regressions with mocked APIs. Final native filters LuaModContracts.*,LuaRuntimePrelude.*,LuaSettingsModule.*,TreeFellerMod.*,ModSettingsRegistry.*,WorldRequiredMods.*. No gameplay/network/performance claim. Compile C++ final pass only,no competing build. Build wrapper may sync profile shaders despite skipped mod/resource sync.

Manual: enabled/disabled,rebind,pause,no player,world switch,restart;content load/recipe/place/save;remote restrictions. Inventory invalid/valid repair,occupied output,repeat tick,swap,full inventory,cursor/close;compare counts.

Owners under src/net/minecraft/mod/: runtime/ModHost.cpp,ModPackageIo.cpp;lua/LuaRuntimePrelude.hpp,LuaHostApi.cpp;runtime/Lua*Bindings.cpp+lua/LuaModApi.cpp;runtime/LuaEventId.hpp,LuaDirectHooks.cpp,LuaEventGlue.*;model/ModModels.cpp,JsonModelParser.cpp,JsonModelBaker.cpp;lua/LuaBlockRegistry.cpp,LuaItemRegistry.cpp,LuaRecipeRegistry.cpp. Shared mods/lib/{settings,commands}/init.lua,mathx.lua. Examples stone_bricks,tree_feller,repair_table,world_profiles,layered_clouds,realtime_sky. Missing capability: inspect owner or state limitation,never invent API.
