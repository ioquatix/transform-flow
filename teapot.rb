
teapot_version "0.8.0"

define_project "Transform Flow" do |project|
	project.add_author "Samuel Williams"
	project.license = "MIT License"
end

define_target "transform-flow" do |target|
	target.depends "Library/Dream"
	target.depends "Library/opencv"
	
	target.provides "Dependencies/TransformFlow"
end

define_configuration "transform-flow" do |configuration|
	configuration[:source] = "https://github.com/dream-framework"

	configuration.import! "project"
	
	configuration.require "opencv"
	
	configuration[:run] = ["Dependencies/TransformFlow"]
end
