extends TextureRect

var http_request: HTTPRequest
var image = Image.new()

func _ready():
	# Configurar o HTTPRequest
	http_request = HTTPRequest.new()
	add_child(http_request)
	http_request.request_completed.connect(_on_image_request_completed)
	
	# Iniciar a captura de imagem
	request_camera_frame()

func request_camera_frame():
	# URL do stream da câmera ESP32
	var url = "http://192.168.1.10/capture"
	http_request.request(url)

func _on_image_request_completed(result, response_code, headers, body):
	if result == HTTPRequest.RESULT_SUCCESS and response_code == 200:
		# Criar uma imagem a partir dos dados recebidos
		var error = image.load_jpg_from_buffer(body)
		
		if error == OK:
			# Criar uma textura a partir da imagem
			var texture = ImageTexture.create_from_image(image)
			
			# Definir a textura no TextureRect
			self.texture = texture
	
	# Agendar próxima captura (opcional, ajuste o intervalo conforme necessário)
	await get_tree().create_timer(0.001).timeout
	request_camera_frame()
