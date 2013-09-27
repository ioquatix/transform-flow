
compile_executable("transform-flow-test-runner") do
	def source_files(environment)
		FileList[root, "**/*.cpp"]
	end
end

copy_files do
	def source_files(environment)
		FileList[root, 'share/**/*']
	end
end
