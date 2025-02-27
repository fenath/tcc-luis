extends Node2D

@onready var websocket: CarWebSocket = $carWebSocket

const IP_ADDRESS: String = '192.168.1.10'
const PORT: int = 4242
var URL := 'ws://' + IP_ADDRESS + ":" + str(PORT)
var last_state

var turn_speed: int = 300

var speeds: Vector2 = Vector2.ZERO

func input(delta: float):
	if Input.is_action_pressed("ui_right"):
		$Car.turn_right(delta)
	elif Input.is_action_pressed("ui_left"):
		$Car.turn_left(delta)
	else:
		$Car.release_wheel(delta)
	
	if Input.is_action_pressed("ui_up"):
		$Car.accelerate(delta)
	elif Input.is_action_pressed("ui_down"):
		$Car.pull_back(delta)
	else:
		$Car.decrease_speed(delta)
	var speed = $Car.get_speed_vector()
	$HSlider.value = speed.x
	$HSlider2.value = speed.y
	
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	input(delta)	
	$CarSpeed.text = str($Car.get_speed())
	$EspFPSLabel.text = 'ESP FPS: ' + $carWebSocket.espFPSAsString
	
func _ready():
	pass
	
func toggle_flash(): 
	websocket.send_text('flash')
	$Label.text = get_message()

func get_message() -> String:
	return websocket.get_message()


func _on_button_pressed() -> void:
	toggle_flash()
	
func _on_h_slider_value_changed(value: float) -> void:
	var floor_value = clamp(floor(value), -100, 100)
	$sliderValue.text = str(floor_value)
	websocket.move_websocket('x', floor_value)

func _on_h_slider_2_value_changed(value: float) -> void:
	var floor_value = clamp(floor(value), -100, 100)
	$sliderValue2.text = str(floor_value)
	websocket.move_websocket('y', floor_value)
