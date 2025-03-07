class_name PacketImageLoader extends Node


func is_valid_jpg_header(data: PackedByteArray) -> bool:
	if data.size() < 4:
		return false
	return (data[0] == 0xFF and data[1] == 0xD8 and data[2] == 0xFF 
			#and (data[3] == 0xE0 or data[3] == 0xDB)
			)

func is_image_complete(data: PackedByteArray) -> bool:
	if data.size() < 5:
		return false
	return (data[0] == 0xFF and data[1] == 0xD8 and data[2] == 0xFF
			and data[-1] == 0xD9 and data[-2] == 0xFF)

func create_texture_from_pool_byte_array(byte_array: PackedByteArray) -> ImageTexture:
		
	if not is_valid_jpg_header(byte_array):
		print("Descartando frame inválido: tamanho ou cabeçalho incorreto")
		return null

	var im: Image = Image.new()
	
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
