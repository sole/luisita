-- Shader constants

SHADER_VERTEX = 1
SHADER_FRAGMENT = 2

trace("hello world")
loadShader("shader_test", SHADER_VERTEX)

function setup()
	trace("This is the setup function")
	setupIsDone()
end

function draw(ticks)
	--trace(ticks)
end

