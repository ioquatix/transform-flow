
compile_library 'TransformFlow' do
	def source_files(environment)
		FileList[root, 'TransformFlow/**/*.cpp']
	end
end

copy_headers do
	def source_files(environment)
		FileList[root, 'TransformFlow/**/*.h']
	end
end

add_application 'Transform Flow' do
	compile_executable 'transform-flow' do
		configure do
			linkflags ["-lTransformFlow"]
		end
	
		def source_files(environment)
			FileList[root, "TransformFlow-Visualisation/**/*.cpp"]
		end
	end
	
	copy_files do
		def source_files(environment)
			FileList[root + "TransformFlow-Visualisation/Resources", '**/*']
		end
	end
end
