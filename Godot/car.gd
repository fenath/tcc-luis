class_name Car extends Node2D


@export var turn_size: float = 0
@export var max_speed: int = 100
@export var accel: int = 600
@export var turn_speed: int = 300

var cur_speed: float
var break_speed: float = 300
var speed_factor: float = 1

func accelerate(delta: float):
	cur_speed += delta * accel
	cur_speed = min(max_speed, cur_speed)

func pull_back(delta: float):
	cur_speed -= delta * accel
	cur_speed = int(max(-max_speed, cur_speed))
	
func turn_left(delta: float) -> void:
	turn_size -= turn_speed * delta
	
func turn_right(delta: float) -> void:
	turn_size += turn_speed * delta
	
	
func release_wheel(delta: float):
	if turn_size == 0:
		return
	if abs(turn_size) <= 5:
		turn_size = 0
		return
	turn_size -= sign(turn_size) * delta * 300 * 2
	
func decrease_speed(delta: float):
	if abs(cur_speed) <= 5:
		cur_speed = 0
		return
	cur_speed -= int(sign(cur_speed) * accel * delta)
	
func move(delta: float) -> void:
	var tr = get_turn_radius()
	if tr != 0:
		speed_factor = clamp(1 - abs(turn_size) / 100.0, 0.7, 1.0)
		var theta = speed_factor * cur_speed * delta / tr
		var offset_x = int(tr * (1 - cos(theta)))
		var offset_y = int(tr * sin(theta))
		position += Vector2(offset_x, -offset_y).rotated(rotation)
		rotate(theta)
	else:
		if speed_factor != 1:
			cur_speed *= speed_factor
			speed_factor = 1
		
		position += Vector2(0, -cur_speed * delta).rotated(rotation)

func get_turn_radius() -> float:
	return (5*abs(turn_size) - 630) *  -sign(turn_size)
	
func get_turn_size() -> int:
	return turn_size

func get_speed_vector() -> Vector2:
	return Vector2(get_turn_size(), (cur_speed * 100/max_speed))
	
func get_speed() -> float:
	return cur_speed * speed_factor

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	move(delta)
	turn_size = clamp(turn_size, -99, 99)
	if abs(turn_size) < 1: turn_size = 0
	if abs(cur_speed) < 1: cur_speed = 0
	
	$TurnRadius.text = 'radius: %.2f' % get_turn_radius()
	$TurnSpeed.text = 'turn_size: %.2f' % turn_size
