#version 420 core

layout(binding = 0, offset = 0) uniform atomic_uint ac;

uniform bool benchmark = true;
out float out_color;

void main()
{
	uint aci = 0;

	if(benchmark)
		aci = 100;
	else
		aci = atomicCounterIncrement(ac);

	out_color = float(aci);
}
