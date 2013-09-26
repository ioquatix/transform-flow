
compile_executable("transform-flow-test-runner") do
	def source_files(environment)
		FileList[root, "**/*.cpp"]
	end
	
	configure do
		append linkflags ["-framework", "Cocoa"]
	end
end

copy_files do
	def source_files(environment)
		FileList[root, 'share/**/*']
	end
end
