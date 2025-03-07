extends Node2D

@export var car: Car = null
@onready var car_to_power_command: CarToPowerCommand = $CarToPowerCommand
@onready var udp_connection: UDPConnection = $Rect/UDPConnection

func input(delta: float):
	if Input.is_action_pressed("ui_right"):
		car.turn_right(delta)
	elif Input.is_action_pressed("ui_left"):
		car.turn_left(delta)
	else:
		car.release_wheel(delta)
	
	if Input.is_action_pressed("ui_up"):
		car.accelerate(delta)
	elif Input.is_action_pressed("ui_down"):
		car.pull_back(delta)
	else:
		car.decrease_speed(delta)
	
func _process(delta: float) -> void:
	input(delta)
	
func _ready():
	car_to_power_command.power_changed.connect(_on_power_changed)

func _on_power_changed(left_power: float, right_power: float):
	udp_connection.enviar_comando('power:%d:%d' %[int(left_power), int(right_power)])
