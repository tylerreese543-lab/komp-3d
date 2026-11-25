local player = scene:FindFirstChild("Player")
local ground = scene:FindFirstChild("Ground")
local camera = scene.CurrentCamera

local jumpableReset = 0.25;
local jumpable = jumpableReset;
local player_velocity = Vector3.new(0, 0, 0);

local camera_angle = Vector3.new(0, 0, 0);
local groundNearby = {};

function getGroundNearby()

	if not player.Collider then return end;
	groundNearby = {};

	for i, obj in pairs(ground:GetChildren()) do
	if (obj.Center - player.Collider.Center).Length < 10 then
			table.insert(groundNearby, obj);
		end
	end
end
function isTouchingAnyGround()
if not player.Collider then return end;

	for i, obj in pairs(groundNearby) do
		if obj:IsTouching(player.Collider) then
			return true;
		end
	end

	return false;
end
function Tick(deltaTime)

camera = scene.CurrentCamera


player_velocity.Y = player_velocity.Y + (-30 * deltaTime);

if jumpable > 0 and KeyboardData:IsKeyDown(Enum.KeyCode.Spacebar) then
	player_velocity.Y = 20;
	jumpable = 0;
end

if (MouseData:IsRawMouseInput()) then
camera_angle = camera_angle + Vector3.new(MouseData.Delta.X / 300, MouseData.Delta.Y / 300, 0);
if (KeyboardData:IsKeyDown(Enum.KeyCode.Q)) then
MouseData:SetMouseInputMode(false)
end
elseif (KeyboardData:IsKeyDown(Enum.KeyCode.E)) then
MouseData:SetMouseInputMode(true)
end

local moveVector = Vector3.new(
	(KeyboardData:IsKeyDown(Enum.KeyCode.D) and 1 or 0) - (KeyboardData:IsKeyDown(Enum.KeyCode.A) and 1 or 0),
	0,
	(KeyboardData:IsKeyDown(Enum.KeyCode.W) and 1 or 0) - (KeyboardData:IsKeyDown(Enum.KeyCode.S) and 1 or 0)
).Unit;

player_velocity = player_velocity + (moveVector * (95 * deltaTime) * Matrix3.RotateY(camera_angle.X));
player_velocity = player_velocity * Vector3.new(0.9, 1, 0.9);
local step = 0.25;
local pv_dt = player_velocity * deltaTime;

jumpable = jumpable - deltaTime; 
if player.Collider then

getGroundNearby();
for i = 1, 4 do
	--X

	player.Position.X = player.Position.X + (pv_dt.X * step);
	player.Collider:UpdateBoundingBox();
	if isTouchingAnyGround() then
		player.Position.X = player.Position.X - (pv_dt.X * step);
		player_velocity.X = 0;
	end
	--Y
	player.Position.Y = player.Position.Y + (pv_dt.Y * step);
	player.Collider:UpdateBoundingBox();
	if isTouchingAnyGround() then
		player.Position.Y = player.Position.Y - (pv_dt.Y * step);
		if (player_velocity.Y < 0) then
			jumpable = jumpableReset;
		end
		player_velocity.Y = 0;
		
	end
	if (player.WorldPosition.Y < -100) then
	player.Position = Vector3.new(0, 10, 0);
	player_velocity.Y = 0;
	end
	--Z
	player.Position.Z = player.Position.Z + (pv_dt.Z * step);
	player.Collider:UpdateBoundingBox();
	if isTouchingAnyGround() then
		player.Position.Z = player.Position.Z - (pv_dt.Z * step);
		player_velocity.Z = 0;
	end
end
end

if (camera and player.Collider) then


	camera.Position = player.Collider.Center + Vector3.new(0, 0, -10) * (Matrix3.RotateXYZ(camera_angle.Y, camera_angle.X,0));
	camera.Rotation = Vector3.new(camera_angle.Y, camera_angle.X, 0);


	player.Rotation = player.Rotation + ((Vector3.new(0, camera_angle.X, 0) - player.Rotation) * 0.1);
	player.Velocity.Value = player_velocity;

end

end