class_name UdpServerRect extends TextureRect

var udp := PacketPeerUDP.new()
const UDP_PORT = 4242
const CHUNK_LENGTH = 1460  # Expected packet size for UDP chunks

var byte_vector: PackedByteArray = PackedByteArray()
var frame_in_progress := false
var last_fps_time := Time.get_ticks_msec()
var nb_frames := 0.0

func _ready() -> void:
	var err = udp.bind(UDP_PORT)
	if err != OK:
		print("Failed to bind UDP port: ", err)
		return
	print("UDP Server listening on port ", UDP_PORT)
	set_process(true)

func _process(_delta: float) -> void:
	render_img()
	
	var current_time = Time.get_ticks_msec()
	if current_time - last_fps_time >= 1000:  # Update every second
		var frame_rate = float(nb_frames) * (1000.0 / float(current_time - last_fps_time))
		display_frame_rate(frame_rate)
		nb_frames = 0
		last_fps_time = current_time

func calculate_delta_time():
	var current_time = Time.get_ticks_msec()
	var result = current_time - last_fps_time
	last_fps_time = current_time
	return result

func create_texture_from_pool_byte_array(byte_array: PackedByteArray) -> ImageTexture:
	var im: Image = Image.new()
		
	if byte_array.size() < 4 or not is_valid_jpg_header(byte_array):
		print("Descartando frame inválido: tamanho ou cabeçalho incorreto")
		return null

	var err = im.load_jpg_from_buffer(byte_array)
	if err != OK:
		print("Error while trying to load image: ", err)
		print("Buffer size: ", byte_array.size())
		if byte_array.size() >= 4:
			print("First bytes: ", byte_array.slice(0, 4))
			print("Last bytes: ", byte_array.slice(-4))
		return null
		
	var im_tx: ImageTexture = ImageTexture.create_from_image(im)
	return im_tx

func is_valid_jpg_header(data: PackedByteArray) -> bool:
	if data.size() < 4:
		return false
	return (data[0] == 0xFF and data[1] == 0xD8 and data[2] == 0xFF and 
			(data[3] == 0xE0 or data[3] == 0xDB))

func render_img() -> void:
	while udp.get_available_packet_count() > 0:
		var pkt = udp.get_packet()
		
		if not frame_in_progress and is_valid_jpg_header(pkt):
			byte_vector = PackedByteArray()
			frame_in_progress = true
		
		if frame_in_progress:
			byte_vector.append_array(pkt)
			
			if pkt.size() >= 2 and pkt[-2] == 0xFF and pkt[-1] == 0xD9:
				frame_in_progress = false
				
				if byte_vector.size() >= 4 and is_valid_jpg_header(byte_vector) and byte_vector[-2] == 0xFF and byte_vector[-1] == 0xD9:
					var imgtexture = create_texture_from_pool_byte_array(byte_vector)
					if imgtexture != null:
						texture = imgtexture
						nb_frames += 1
				else:
					frame_in_progress = false
					byte_vector.clear()

func display_frame_rate(fps: float) -> void:
	$FrameRate.text = 'frames recebidos: ' + str(fps)


func _exit_tree() -> void:
	udp.close()
