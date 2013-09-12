
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
