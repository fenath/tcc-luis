class_name Car extends CharacterBody2D


@export var max_speed: float = 50.0
@export var accel: float = 2.0
@export var turn_speed: float = 4.0
@export var max_turn_angle: float = 45

const BASE_TURN_LENGTH = 50.0
const SPEED_FACTOR := 1.5
const ACCEL_FACTOR := 300.0

var cur_speed: float
var break_speed: float = 15.0
var friction: float = 1

var turn_angle: float
var turn_ratio: float: set = set_turn_ratio

func set_turn_ratio(new_ratio: float) -> void:
	turn_ratio = clamp(new_ratio, -1, +1)
	turn_angle = remap(turn_ratio, -1, 1, -max_turn_angle, max_turn_angle)

func accelerate(delta: float):
	cur_speed += delta * accel * ACCEL_FACTOR
	cur_speed = min(max_speed * friction, cur_speed)

func pull_back(delta: float):
	cur_speed -= delta * accel  * ACCEL_FACTOR
	cur_speed = int(max(-max_speed, cur_speed))
	
func turn_left(delta: float) -> void:
	turn_ratio -= turn_speed * delta
	
func turn_right(delta: float) -> void:
	turn_ratio += turn_speed * delta
	
func release_wheel(delta: float):
	turn_ratio = lerp(turn_ratio, 0.0, delta * turn_speed * 2)
	
func decrease_speed(delta: float):
	cur_speed = lerp(cur_speed, 0.0, delta * break_speed)

func get_turn_radius(turn_angle_rad) -> float:
	# retorna o raio de giro de acordo com o angulo de giro
	# como o comprimento de um arco é dado por 
	# Arco = Angulo * Raio
	# Raio = Arco / Angulo
	if turn_angle_rad == 0:
		return INF
	return BASE_TURN_LENGTH / turn_angle_rad

func get_speed() -> float:
	return cur_speed
	
func _physics_process(delta: float) -> void:
	if turn_ratio == 0:
		friction = lerp(friction, 1.0, delta * 10.0)
		cur_speed *= friction
		self.velocity = cur_speed * SPEED_FACTOR * Vector2.UP.rotated(rotation)
	else:
		var turn_radius = get_turn_radius(deg_to_rad(turn_angle))
		friction = clamp(1 - abs(turn_ratio), 0.9, 1.0)
		cur_speed *= friction
		
		# Como arc = angle * radius, angle = arc / radius
		# arc = velocidade * delta
		var theta = velocity.length() * delta / turn_radius
		
		rotate(theta * sign(cur_speed))
		self.velocity = cur_speed * SPEED_FACTOR * Vector2.UP.rotated(self.rotation)
			
	move_and_slide()

func _process(delta: float) -> void:
	$TurnRadius.text = 'angle: %d' % int(turn_angle)
	$TurnSpeed.text = 'turn_ratio: %+.2f' % turn_ratio
	$CarSpeed.text = str(int(self.get_speed()))
