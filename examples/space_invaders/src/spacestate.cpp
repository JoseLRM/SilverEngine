#include "spacestate.h"

void SpaceState::shipShot(ShipComponent& ship, f32 dir, bool condition, f32 dt)
{
	ship.shotTime += dt;

	f32 rate = f32_max;

	switch (ship.bulletType)
	{
	case BulletType_Noob:
		rate = 0.5f;
		break;
	case BulletType_Shotgun:
		rate = 1.f;
		break;
	case BulletType_MachineGun:
		rate = 0.01f;
		break;
	case BulletType_Sniper:
		rate = 0.7f;
		break;
	}

	if (condition) {

		while (ship.shotTime >= rate) {
			ship.shotTime -= rate;

			switch (ship.bulletType)
			{
			case BulletType_Noob:
				ship.acc -= 0.1f;
				createBullet(ship.entity, 0.1f, dir, 0.5f, 4.f, 2.f, false);
				break;

			case BulletType_Shotgun:
				ship.acc -= 0.6f;

				foreach(i, 30)
					createBullet(ship.entity, 1.f, dir + random.gen_f32(-PI * 0.25f, PI * 0.25f), 0.5f, 0.3f, 1.f, false);
				break;

			case BulletType_MachineGun:
				ship.acc -= 0.01f;
				createBullet(ship.entity, 1.f, dir + random.gen_f32(-PI * 0.05f, PI * 0.05f), 0.5f, 4.f, 0.5f, false);
				break;

			case BulletType_Sniper:
				ship.acc -= 0.5f;
				createBullet(ship.entity, 2.f, dir, 0.5f, 4.f, 4.f, true);
				break;
			}
		}
	}
	else ship.shotTime = std::min(ship.shotTime, rate);
}

Result createAssets(SpaceState& s)
{
	// TEXTURES
	TextureAsset explosionTex;

	// SHIPS
	svCheck(s.sprite_Player.texture.loadFromFile("images/ships0.png"));
	s.sprite_Player.texCoord = { 2.f / 3.f, 2.f / 3.f, 3.f / 3.f, 3.f / 3.f };

	svCheck(s.sprite_Kamikaze.texture.loadFromFile("images/ships0.png"));
	s.sprite_Kamikaze.texCoord = { 0.f / 3.f, 1.f / 3.f, 1.f / 3.f, 2.f / 3.f };

	svCheck(s.sprite_Shooter.texture.loadFromFile("images/ships0.png"));
	s.sprite_Shooter.texCoord = { 1.f / 3.f, 0.f / 3.f, 2.f / 3.f, 1.f / 3.f };

	svCheck(s.sprite_Daddy.texture.loadFromFile("images/ships0.png"));
	s.sprite_Daddy.texCoord = { 0.f / 3.f, 2.f / 3.f, 1.f / 3.f, 3.f / 3.f };

	// ASTEROIDS

	svCheck(s.sprite_Asteroid0.texture.loadFromFile("images/asteroid.png"));
	s.sprite_Asteroid1 = s.sprite_Asteroid0;
	s.sprite_Asteroid2 = s.sprite_Asteroid0;
	s.sprite_Asteroid3 = s.sprite_Asteroid0;

	s.sprite_Asteroid0.texCoord = texcoord_from_atlas(2u, 2u, 0u);
	s.sprite_Asteroid1.texCoord = texcoord_from_atlas(2u, 2u, 1u);
	s.sprite_Asteroid2.texCoord = texcoord_from_atlas(2u, 2u, 2u);
	s.sprite_Asteroid3.texCoord = texcoord_from_atlas(2u, 2u, 3u);

	svCheck(asset_refresh());

	return Result_Success;
}

Result SpaceState::initialize()
{
	ecs_component_register<ShipComponent>("Ship");
	ecs_component_register<ProjectileComponent>("Projectile");
	ecs_component_register<AsteroidComponent>("Asteroid");
	ecs_component_register<BulletComponent>("Bullet");

	if (result_fail(createAssets(*this))) {
		SV_LOG_ERROR("Can't create the assets");
	}

	ecs_create(&ecs);

	random.setSeed(timer_now().toMillis_u32());

	// Create player
	player = createShip({}, ShipType_Player);


	shipGenerator.addDefaultGenerations();

	// Generate background particles
	{
		background = ecs_entity_create(ecs);
		// TODO
	}

	// TEMP
	// Create font
	svCheck(font_create(font, "C:/Windows/Fonts/arial.ttf", 128.f, 0));

	svCheck(graphics_offscreen_create(1920u, 1080u, &offscreen));

	// Adjust Camera
	projection.projectionType = ProjectionType_Orthographic;
	projection.width = 1080.f / 15.f;
	projection.height = 720.f / 15.f;
	projection.near = -100.f;
	projection.far = 100.f;
	projection.updateMatrix();

	return Result_Success;
}

void SpaceState::update(f32 dt)
{
	// Asteroid Generator
	{
		static f32 timer = 0.f;
		static f32 asteroidRate = 2.f;
		static f32 rate = asteroidRate;

		timer += dt;

		if (timer >= rate) {
			timer -= rate;

			rate = asteroidRate + random.gen_f32(-0.2f, 0.2f);

			auto [pos, dir] = genPos();

			createAsteroid(pos, random.gen_f32(1.3f, 4.f), random.gen_f32(0.1f, 0.5f), dir);
		}
	}

	shipGenerator.generateShips(*this, dt);

	// Player controller
	if (player != SV_ENTITY_NULL) {
		Transform trans = ecs_entity_transform_get(ecs, player);

		ShipComponent& ship = *ecs_component_get<ShipComponent>(ecs, player);
		v2_f32 pos = trans.getLocalPosition().getVec2();
		v2_f32 scale = trans.getLocalScale().getVec2();

		v2_f32 mousePos = input_mouse_position() * camSize();
		v2_f32 toMouse = mousePos - pos;
		f32 dir = toMouse.angle();

		trans.setEulerRotation({ 0.f, 0.f, dir });

		if (input_mouse(SV_MOUSE_RIGHT)) {
			ship.acc += dt;
		}

		ship.bulletType = BulletType_Shotgun;
		shipShot(ship, dir, input_mouse(SV_MOUSE_LEFT), dt);
	}
	else {
		static f32 exitCount = 0.f;
		exitCount += dt;

		if (exitCount > 1.5f) {
			engine.close_request = true;
		}
	}

	// Ship AI
	{
		EntityView<ShipComponent> ships(ecs);

		v2_f32 pPos;
		if (player != SV_ENTITY_NULL) {
			Transform pTrans = ecs_entity_transform_get(ecs, player);
			pPos = pTrans.getLocalPosition().getVec2();
		}

		for (ShipComponent& ship : ships) {

			if (ship.shipType == ShipType_Player)
				continue;

			Transform trans = ecs_entity_transform_get(ecs, ship.entity);

			switch (ship.shipType)
			{
			case ShipType_Kamikaze:
				if (player != SV_ENTITY_NULL) {

					v2_f32 to = pPos - trans.getWorldPosition().getVec2();

					ship.acc += dt * random.gen_f32(0.2f, .8f);
					trans.setEulerRotation({ 0.f, 0.f, to.angle() });

					ship.color.r = u8(abs(sin(timer_now() * 7.f + f32(ship.entity) * 10.32f)) * 255.f) * 0.4f;
				}
				break;

			case ShipType_Shooter:
				if (player != SV_ENTITY_NULL) {

					v2_f32 to = pPos - trans.getWorldPosition().getVec2();

					if (to.length() < 20.f) {
						f32 dir = to.angle();
						trans.setEulerRotation({ 0.f, 0.f, dir });
						shipShot(ship, dir, true, dt);
					}
					else {
						ship.acc += dt * 0.8f;
						trans.setEulerRotation({ 0.f, 0.f, to.angle() });
					}
				}
				break;

			case ShipType_Daddy:
			{
				v2_f32 pos = trans.getWorldPosition().getVec2();
				v2_f32 to = pPos - pos;

				if (to.length() > 15.f) {
					f32 dir = to.angle();
					trans.setEulerRotation({ 0.f, 0.f, dir });
					ship.acc += dt * 0.3f;
				}

				ship.shotTime += dt;

				if (ship.shotTime > 2.f)
				{
					ship.shotTime -= 2.f;
					createShip(pos, ShipType_Kamikaze);
				}

			}	break;
			}
		}
	}

	// DESTROY ENTITIES
	{
		// Destroy Bullets
		{
			EntityView<BulletComponent> bullets(ecs);

			for (BulletComponent& bullet : bullets) {

				bullet.lifeTime -= dt;

				if (bullet.lifeTime <= 0.f || bullet.damage <= 0.f) {
					destroyBullet(bullet);
					break;
				}
			}
		}

		// Destroy Ships
		{
			EntityView<ShipComponent> ships(ecs);

			for (ShipComponent& s : ships) {

				if (s.health <= 0.f) {
					destroyShip(s);
					continue;
				}
			}
		}
		// Destroy Asteroids
		{
			EntityView<AsteroidComponent> asteroids(ecs);

			for (AsteroidComponent& a : asteroids) {

				Transform trans = ecs_entity_transform_get(ecs, a.entity);
				v2_f32 pos = trans.getWorldPosition().getVec2();
				f32 scale = trans.getWorldScale().getVec2().length();

				if (a.health <= 0.f) {
					destroyAsteroid(a);
					continue;
				}

				if (!inScreen(pos, scale * 0.5f)) {
					destroyAsteroid(a);
					continue;
				}
			}
		}
	}
}

void SpaceState::fixedUpdate()
{
	// Move projectiles
	{
		EntityView<ProjectileComponent> projectiles(ecs);

		for (ProjectileComponent& prj : projectiles) {

			Transform trans = ecs_entity_transform_get(ecs, prj.entity);

			v2_f32 vel = { prj.vel, 0.f };
			vel.setAngle(prj.dir);

			trans.setPosition(trans.getLocalPosition() + vel.getVec3());
		}
	}

	// Rotate asteroids
	{
		EntityView<AsteroidComponent> asteroids(ecs);

		for (AsteroidComponent& a : asteroids) {

			Transform trans = ecs_entity_transform_get(ecs, a.entity);
			v3_f32 rot = trans.getLocalEulerRotation();
			rot.z += a.rotVel;
			trans.setEulerRotation(rot);
		}
	}

	// Move ships
	{
		EntityView<ShipComponent> ships(ecs);

		for (ShipComponent& ship : ships) {

			Transform trans = ecs_entity_transform_get(ecs, ship.entity);

			f32 dir = trans.getWorldEulerRotation().z;

			// Apply acc
			if (ship.acc != 0.f) {

				v2_f32 acc = { ship.acc, 0.f };
				acc.setAngle(dir);
				ship.vel += acc * (ship.acc > 0.f ? 1.f : -1.f);
				ship.acc = 0.f;
			}

			// Apply vel and friction
			trans.setPosition(trans.getLocalPosition() + ship.vel.getVec3());
			ship.vel *= 0.98f;

			// Velocity limits
			{
				f32 v = ship.vel.length();

				if (v < 0.005f)
					ship.vel = 0.f;
				else if (v > ship.maxVel) {
					ship.vel.normalize();
					ship.vel *= ship.maxVel;
				}
			}

			// Move ships when goes out of screen
			{
				v2_f32 pos = trans.getLocalPosition().getVec2();
				f32 scale = trans.getLocalScale().getVec2().length() * 0.5f;

				i32 x = inScreenX(pos.x, scale);
				i32 y = inScreenY(pos.y, scale);

				v2_f32 cam = 0.5f * camSize();

				if (x == -1) pos.x = cam.x;
				else if (x == 1) pos.x = -cam.x;
				if (y == -1) pos.y = cam.y;
				else if (y == 1) pos.y = -cam.y;

				trans.setPosition(pos.getVec3());
			}
		}
	}

	// Collisions
	{
		// Bullets - Asteroids
		collideObjects<AsteroidComponent, BulletComponent>(0.5f, 0.35f,
			[](AsteroidComponent& a, BulletComponent& b)
		{
			f32 damage = std::min(a.health, b.damage);
			b.last = a.entity;

			a.health -= damage;

			if (!b.penetrate)
				b.damage -= damage;

			if (a.health == 0.f) {
				return false;
			}
			return true;
		});

		// Asteroids - Ships
		collideObjects<ShipComponent, AsteroidComponent>(0.35f, 0.35f,
			[this](ShipComponent& s, AsteroidComponent& a)
		{
			if (s.shipType == ShipType_Daddy) {
				destroyAsteroid(a);
			}
			else destroyShip(s);
			return false;
		});

		// Ships - Ships
		collideObjects<ShipComponent, ShipComponent>(0.35f, 0.35f,
			[this](ShipComponent& s0, ShipComponent& s1)
		{
			if (s0.entity == s1.entity || s0.shipType == ShipType_Daddy || s1.shipType == ShipType_Daddy)
				return true;

			destroyShip(s0);
			destroyShip(s1);
			return false;
		});

		// Bullets - Bullets
		collideObjects<BulletComponent, BulletComponent>(0.5f, 0.5f,
			[this](BulletComponent& b0, BulletComponent& b1)
		{
			if (b0.from == b1.from)
				return true;

			destroyBullet(b0);
			destroyBullet(b1);
			return false;
		});

		// Ships - Bullets
		collideObjects<ShipComponent, BulletComponent>(0.35f, 0.35f,
			[this](ShipComponent& s, BulletComponent& b)
		{
			if (b.from == s.entity)
				return true;

			f32 damage = std::min(s.health, b.damage);
			b.last = s.entity;

			s.health -= damage;

			if (!b.penetrate)
				b.damage -= damage;

			if (s.health == 0.f) {
				return false;
			}
			return true;
		});
	}
}

void SpaceState::render()
{
	CommandList cmd = graphics_commandlist_begin();

	// Clear screen
	graphics_image_clear(offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

	graphics_viewport_set(offscreen, 0u, cmd);
	graphics_scissor_set(offscreen, 0u, cmd);

	sprite_instances.reset();

	// Draw Bullets
	{
		EntityView<BulletComponent> bullets(ecs);
		XMMATRIX tm;

		for (BulletComponent& b : bullets) {

			Transform trans = ecs_entity_transform_get(ecs, b.entity);
			tm = trans.getWorldMatrix();

			sprite_instances.emplace_back(tm, v4_f32{ 0.f, 0.f, 1.f, 1.f }, nullptr, Color::Blue());
		}
	}

	// Draw Ships
	{
		EntityView<ShipComponent> ships(ecs);
		XMMATRIX tm;

		for (ShipComponent& ship : ships) {

			Transform trans = ecs_entity_transform_get(ecs, ship.entity);
			tm = trans.getWorldMatrix();

			v4_f32 texCoord = { 0.f, 0.f, 1.f, 1.f };
			GPUImage* image = nullptr;

			switch (ship.shipType)
			{
			case ShipType_Player:
				image = sprite_Player.texture.get();
				texCoord = sprite_Player.texCoord;
				break;

			case ShipType_Kamikaze:
				image = sprite_Kamikaze.texture.get();
				texCoord = sprite_Kamikaze.texCoord;
				break;

			case ShipType_Shooter:
				image = sprite_Shooter.texture.get();
				texCoord = sprite_Shooter.texCoord;
				break;

			case ShipType_Daddy:
				image = sprite_Daddy.texture.get();
				texCoord = sprite_Daddy.texCoord;
				break;
			}

			sprite_instances.emplace_back(tm, texCoord, image, Color::White());
		}
	}

	// Draw Asteroids
	{
		EntityView<AsteroidComponent> asteroids(ecs);
		XMMATRIX tm;

		for (AsteroidComponent& a : asteroids) {

			Transform trans = ecs_entity_transform_get(ecs, a.entity);
			tm = trans.getWorldMatrix();

			v4_f32 texCoord;
			GPUImage* image;

			switch (a.asteroidID)
			{
			case 0:
				texCoord = sprite_Asteroid0.texCoord;
				image = sprite_Asteroid0.texture.get();
				break;

			case 1:
				texCoord = sprite_Asteroid1.texCoord;
				image = sprite_Asteroid0.texture.get();
				break;

			case 2:
				texCoord = sprite_Asteroid2.texCoord;
				image = sprite_Asteroid0.texture.get();
				break;

			case 3:
				texCoord = sprite_Asteroid3.texCoord;
				image = sprite_Asteroid0.texture.get();
				break;

			default:
				continue;
			}

			sprite_instances.emplace_back(tm, texCoord, image, Color{ 115u, 77u, 38u, 255u });
		}
	}

	// Draw Sprites
	draw_sprites(sprite_instances.data(), sprite_instances.size(), projection.projectionMatrix, offscreen, cmd);

	// TODO: PostProcessing
	//postprocess_bloom(
	//	gBuffer.offscreen, 
	//	GPUImageLayout_RenderTarget, 
	//	GPUImageLayout_RenderTarget, 
	//	gBuffer.emissive,
	//	GPUImageLayout_RenderTarget,
	//	GPUImageLayout_RenderTarget,
	//	0.9f, 
	//	80.f, 
	//	5u, 
	//	cmd
	//);

	// TEMP Draw some text
	std::string score = "Score: ";
	score += std::to_string(timer_now().toSeconds_u32());
	draw_text(score.c_str(), 0.01f, 0.99f, 0.3f, 1u, 0.05f, projection.width / projection.height, TextSpace_Normal, TextAlignment_Left, &font, offscreen, cmd);

	// Present
	graphics_present(engine.window, offscreen, GPUImageLayout_RenderTarget, cmd);
}

Result SpaceState::close()
{
	ecs_destroy(ecs);

	graphics_destroy(offscreen);

	return Result_Success;
}

CameraProjection& SpaceState::getCameraProjection()
{
	return projection;
}

