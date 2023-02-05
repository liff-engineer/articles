add_library(ImplValue SHARED)

target_sources(ImplValue 
	PRIVATE ImplValue.h 
	Printer.h Printer.cpp 
	ImplValue.natvis
)

set_target_properties(ImplValue PROPERTIES 
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_executable(ImplValueApp 
	ImplValueApp.cpp 
)

target_link_libraries(ImplValueApp
	PRIVATE ImplValue
)
