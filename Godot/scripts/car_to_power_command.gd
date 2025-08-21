class_name CarToPowerCommand extends Node


@export var car: Car

signal power_changed(left: float, right: float)

var power_left : float
var power_right : float

func _process(delta: float) -> void:
	var left = car.cur_speed
	var right = car.cur_speed
	
	# if car turning to right
	if car.turn_ratio > 0:
		right *= (1-abs(car.turn_ratio))
	
	# if car turning to left, left power decreases
	if car.turn_ratio < 0:
		left *= (1-abs(car.turn_ratio))
		
	if (left != power_left) or (right != power_right):
		power_left = left
		power_right = right
		power_changed.emit(power_left, power_right)
		print("left: %d | right: %d" % [power_left, power_right])
