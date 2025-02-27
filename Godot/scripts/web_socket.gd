class_name CarWebSocket extends Node

@onready var websocket: WebSocketPeer = WebSocketPeer.new()
@export var max_attempts: int = 5

@export var IP_ADDRESS: String = '192.168.1.10'
@export var PORT: int = 4242

@onready var URL := 'ws://' + IP_ADDRESS + ":" + str(PORT)
var last_state
var attempts: int = 0
var espFPSAsString: String = ''

@onready var is_connected: bool = false

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	websocket.poll()
	
	get_fps()
	
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
	
func connect_websocket():
	var err := websocket.connect_to_url(URL)
	
	if err != OK:
		print('could not connect to server ' + URL)
		return err

	last_state = websocket.get_ready_state()
	print('connected, status: ')
	print(last_state)
	return OK
	
func retry_connection() -> void:
	attempts += 1
	$reconnectTimer.wait_time = 1
	$reconnectTimer.start()	
	
func _ready():
	return connect_websocket()
	
func toggle_flash(): 
	websocket.send_text('flash')
	$Label.text = get_message()
	
func get_fps() -> void:
	self.send_command('fps')
	var msg = get_message()
	if msg!= 'null':
		espFPSAsString = msg

func get_message() -> String:
	if websocket.get_available_packet_count() < 1:
		return 'null'
	var pkt := websocket.get_packet()
	if websocket.was_string_packet():
		return pkt.get_string_from_utf8()
	return bytes_to_var(pkt)


func _on_button_pressed() -> void:
	toggle_flash()


func send_command(cmd: String) -> void:
	if websocket.get_ready_state() == WebSocketPeer.STATE_OPEN:
		websocket.send_text(cmd)
	else:
		retry_connection()

func move_websocket(axis: String, value: int) -> void:
	var cmd = get_mv_cmd(axis, value)
	send_command(cmd)
	
func get_mv_cmd(axis: String, value: int):
	var sinal: String = '+'
	if value < 0:
		sinal = '-' 
	return 'mv'+axis+sinal+str(abs(value))

func _on_reconnect_timer_timeout() -> void:
	print('retrying')
	if attempts >= max_attempts:
		print('connection failed after %d attempts'% attempts)
		return
	
	var err = connect_websocket()
	if err != OK:
		print('connection failed, retrying')
		retry_connection()
	else:
		attempts = 0
		
