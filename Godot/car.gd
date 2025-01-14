class_name Car extends Node2D


@export var turn_size: float = 0
@export var max_speed: int = 500
@export var accel: int = 600

var cur_speed: float
var break_speed: float = 300
var speed_factor: float = 1

func accelerate(delta: float):
	cur_speed += delta * accel
	cur_speed = min(max_speed, cur_speed)

func pull_back(delta: float):
	cur_speed -= delta * accel
	cur_speed = max(-max_speed, cur_speed)
	
func decrease_speed(delta: float):
	if abs(cur_speed) == 1:
		cur_speed = 0
		return
	cur_speed -= sign(cur_speed) * accel * delta
	
func move(delta: float) -> void:
	var tr = get_turn_radius()
	if tr != 0:
		speed_factor = clamp(1 - abs(turn_size) / 100.0, 0.7, 1.0)
		var theta = speed_factor * cur_speed * delta / tr
		var offset_x = tr * (1 - cos(theta))
		var offset_y = tr * sin(theta)
		position += Vector2(offset_x, -offset_y).rotated(rotation)
		rotate(theta)
	else:
		if speed_factor != 1:
			cur_speed *= speed_factor
			speed_factor = 1
		
		position += Vector2(0, -cur_speed * delta).rotated(rotation)

func get_turn_radius() -> float:
	return (5*abs(turn_size) - 630) *  -sign(turn_size)
	
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
