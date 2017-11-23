

find_library(NVIDIA_OPENGL_gl_LIBRARY
  GL
  PATHS /usr/lib/nvidia-331 /usr/lib/nvidia-340 /usr/lib/nvidia-346 
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
)


get_filename_component(NVIDIA_OPENGL_gl_LIBRARY_PATH 
${NVIDIA_OPENGL_gl_LIBRARY} 
DIRECTORY)

