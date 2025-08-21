class_name UDPConnection extends Node

var udp_server = PacketPeerUDP.new()
var esp_ip = ""  # Para armazenar o IP do ESP32 quando ele se conectar
var server_port = 4242  # Porta do servidor
var esp_port = 4211  # Porta do ESP32

@export var HANDSHAKE_INTERVAL: float = 1.0
@onready var wait_handshake: float = HANDSHAKE_INTERVAL

signal img_rendered

var ping_cmd := ''

enum PACKET_TYPE {
	Img,
	Txt,
	None
}

@onready var image_loader: PacketImageLoader = PacketImageLoader.new()
var image_buffer := {}
var last_image_length := 0
var current_image_id: int = -1

func _ready():
	var err = udp_server.bind(server_port)
	if err != OK:
		print("Erro ao iniciar o servidor UDP!")
		return
	print("Servidor UDP iniciado na porta ", server_port)

func get_packet_type(pkt: PackedByteArray) -> PACKET_TYPE:
	if pkt.size() < 1:
		return PACKET_TYPE.None
	if pkt[0] == 0x49: # 'I'
		return PACKET_TYPE.Img
	if pkt[0] == 0x54: # 'T'
		return PACKET_TYPE.Txt
	return PACKET_TYPE.None
	
func _handle_handshake(delta: float) -> void:
	# nao precisa disso se o próprio esp fizer este broadcast, a dúvida é quem
	# é que fica responsável para isso
	wait_handshake -= delta
	if wait_handshake <= 0:
		wait_handshake = HANDSHAKE_INTERVAL
		enviar_comando_broadcast("myip:godot")
	
	
func debug_print_packet(packet: PackedByteArray) -> void:
	var pkt_type = get_packet_type(packet)
	if pkt_type == PACKET_TYPE.Img:
		var img_id = (packet[1] << 8) | packet[2]
		var packet_number = (packet[3] << 8) | packet[4] 
		print('pacote: id (%d) seccao %d' % [img_id, packet_number])
		return
	
	var hex_string = ''
	for byte in packet:
		hex_string += '0x%x ' % byte
	print('pacote: Tamanho %d - %s' % [packet.size(), hex_string])

func process_packets() -> void:
	while udp_server.get_available_packet_count() > 0:
		var packet: PackedByteArray = udp_server.get_packet()
		var sender_ip = udp_server.get_packet_ip()
		var sender_port = udp_server.get_packet_port()

		esp_ip = sender_ip  # Atualiza IP do ESP32 quando recebe algo
		
		var pkt_type = get_packet_type(packet)
		#debug_print_packet(packet)
		
		if pkt_type == PACKET_TYPE.None:
			print('pacote desconhecido ignorado, tamanho: %s, primeiro byte: %s' % [str(packet.size()), str(packet[0])])
		elif pkt_type == PACKET_TYPE.Txt:
			process_text_packet(sender_ip, sender_port, packet)
			
		elif pkt_type == PACKET_TYPE.Img:
			if packet.size() < 5:
				print('pacote de imagem incorreto, ignorando...')
				continue
				
			var image_id = (packet[1] << 8) | packet[2]
			var packet_number = (packet[3] << 8) | packet[4]
			var packet_data = packet.slice(5)
			
			if current_image_id != image_id:
				current_image_id = image_id
				image_buffer.clear()
			
			image_buffer[packet_number] = packet_data
			
			if image_buffer.size() > 1:
				var combined_image = combine_image_packets()
				if image_loader.is_image_complete(combined_image):
					render_img(combined_image)
					image_buffer.clear()
					
func process_text_packet(sender_ip: String, sender_port: int, packet: PackedByteArray):
	var message = packet.slice(1).get_string_from_utf8()
	#print("Recebido de %s:%d -> %s" % [sender_ip, sender_port, message])
	
	# Responder conforme o conteúdo recebido
	if message == "":
		return
	elif message.begins_with("pong"):
		calcular_ping(message)
	elif "fps" in message:
		print("FPS recebido: ", message)
	elif "esp_ping" in message:
		enviar_comando("myip:godot")
	else:
		print("Outro dado recebido. " + message)
		

func _process(delta):
	_handle_handshake(delta)
	process_packets()

func combine_image_packets() -> PackedByteArray:
	var combined = PackedByteArray()
	var keys = image_buffer.keys()
	keys.sort()
	
	for key in keys:
		combined.append_array(image_buffer[key])
		
	return combined

func render_img(pkt: PackedByteArray) -> void:
	if ! image_loader.is_valid_jpg_header(pkt):
		return
		
	var imgtexture = image_loader.create_texture_from_pool_byte_array(pkt)
	if imgtexture != null:
		self.texture = imgtexture
		img_rendered.emit()

func enviar_comando(comando: String):
	if esp_ip != "":
		udp_server.set_dest_address(esp_ip, esp_port)  # Define IP e porta do ESP32
		udp_server.put_packet(comando.to_utf8_buffer())
		#print("Enviado: ", comando)
	else:
		print("ESP32 ainda não foi detectado.")
		
func enviar_comando_broadcast(comando: String) -> void:
	udp_server.set_dest_address('192.168.1.255', esp_port)
	udp_server.put_packet(comando.to_utf8_buffer())	
		
func enviar_ping() -> void:
	var now = Time.get_ticks_msec()
	ping_cmd = "ping:" + str(now)
	enviar_comando(ping_cmd)
	
func calcular_ping(msg_retorno: String) -> void:
	if !msg_retorno.contains(ping_cmd):
		print('mensagem de retorno inválida')
		return
	var partes = msg_retorno.split(":")
	var now = Time.get_ticks_msec()
	var time_sent = int(partes[-1])
	var latency = now - time_sent
	print("ping: " + str(latency) + " ms")

func _input(event):
	if event is InputEventKey and event.pressed:
		if event.is_action_pressed("get_fps"):
			enviar_comando("fps")
		if event.is_action_pressed("get_ping"):
			enviar_ping()
		if event.is_action_pressed("get_power"):
			enviar_comando("power:50:50")
