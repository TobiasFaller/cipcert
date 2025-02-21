smoketest:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && make -C build -j clean all
	./build/bin/dimcert examples/model.dimspec examples/witness.dimspec examples/reset.cir examples/transition.cir examples/property.cir examples/base.cir examples/step.cir
	@printf "Reset      (E(s) A(s'\\\\s) i ^ u => i' ^ u')             "; ../build/quabs/quabs "examples/reset.cir"; true
	@printf "Transition (E(s) A(s'\\\\s) t ^ u0 ^ u1 ^ u' => t' ^ u1') "; ../build/quabs/quabs "examples/transition.cir"; true
	@printf "Property   (E(s) A(s'\\\\s) u ^ u' => (-g => -g'))        "; ../build/quabs/quabs "examples/property.cir"; true
	@printf "Base       (E(s') i' ^ u' => -g')                      "; ../build/quabs/quabs "examples/base.cir"; true
	@printf "Step       (E(s') -g0' ^ t' ^ u0' ^ u1' => -g1')       "; ../build/quabs/quabs "examples/step.cir"; true
