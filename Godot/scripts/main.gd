extends Node2D

@export var car: Car = null
@onready var car_to_power_command: CarToPowerCommand = $CarToPowerCommand
@onready var udp_connection: UDPConnection = $Rect/UDPConnection

var quadros_contados := 0

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


func _on_button_pressed() -> void:
	_on_power_changed(50,50)
	await get_tree().create_timer(3).timeout
	_on_power_changed(0,0)


func _on_btn_ping_pressed() -> void:
	udp_connection.enviar_ping()

func _on_btn_ping_2_pressed() -> void:
	print("Ultima imagem contem pacotes:" + str(udp_connection.last_image_length))

func _on_btn_contar_quadros_pressed() -> void:
	self.quadros_contados = 0
	udp_connection.img_rendered.connect(inc_quadro)
	await get_tree().create_timer(3 * 60).timeout
	udp_connection.img_rendered.disconnect(inc_quadro)

func inc_quadro() -> void:
	self.quadros_contados += 1
	$btnContarQuadros.text = 'Contar Quadros\n(%d)' % [self.quadros_contados]
