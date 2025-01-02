extends Node2D

@onready var websocket: WebSocketPeer = WebSocketPeer.new()

const IP_ADDRESS: String = '192.168.1.32'
const PORT: int = 4242
var URL := 'ws://' + IP_ADDRESS + ":" + str(PORT)
var last_state

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	# Call this in _process or _physics_process. Data transfer and state updates
	# will only happen when calling this function.
	websocket.poll()

	# get_ready_state() tells you what state the socket is in.
	var state = websocket.get_ready_state()

	# WebSocketPeer.STATE_OPEN means the socket is connected and ready
	# to send and receive data.
	if state == WebSocketPeer.STATE_OPEN:
		while websocket.get_available_packet_count():
			print("Got data from server: ", websocket.get_packet().get_string_from_utf8())

	# WebSocketPeer.STATE_CLOSING means the socket is closing.
	# It is important to keep polling for a clean close.
	elif state == WebSocketPeer.STATE_CLOSING:
		pass

	# WebSocketPeer.STATE_CLOSED means the connection has fully closed.
	# It is now safe to stop polling.
	elif state == WebSocketPeer.STATE_CLOSED:
		# The code will be -1 if the disconnection was not properly notified by the remote peer.
		var code = websocket.get_close_code()
		print("WebSocket closed with code: %d. Clean: %s" % [code, code != -1])
		set_process(false) # Stop processing.
	
func _ready():
	var err := websocket.connect_to_url(URL)
	if err != OK:
		print('could not connect to server ' + URL)
		return err

	last_state = websocket.get_ready_state()
	print(last_state)
	return OK
	
func toggle_flash(): 
	websocket.send_text('flash')
	$Label.text = get_message()

func get_message() -> String:
	if websocket.get_available_packet_count() < 1:
		return 'null'
	var pkt := websocket.get_packet()
	if websocket.was_string_packet():
		return pkt.get_string_from_utf8()
	return bytes_to_var(pkt)


func _on_button_pressed() -> void:
	toggle_flash()


func _on_h_slider_value_changed(value: float) -> void:
	# send value to flash
	var floor_value = floor(value)
	if floor_value > 99:
		floor_value = 99
	if floor_value < 0:
		floor_value = 0
	$sliderValue.text = str(floor_value)
	websocket.send_text('mvx+'+str(floor_value))
	pass # Replace with function body.
