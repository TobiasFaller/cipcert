smoketest:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DASAN=ON && make -C build -j clean all
	./build/bin/dimcert examples/model.dimspec examples/witness.dimspec examples/1_reset.cir examples/2_transition.cir examples/3_property.cir examples/4_base.cir examples/5_step.cir
	@printf "Reset      (E(s) A(s'\\\\s) -(i ^ u => i' ^ u'))             "; ../build/quabs/quabs "examples/1_reset.cir"; true
	@printf "Transition (E(s) A(s'\\\\s) -(t ^ u0 ^ u1 ^ u' => t' ^ u1')) "; ../build/quabs/quabs "examples/2_transition.cir"; true
	@printf "Property   (E(s) A(s'\\\\s) -(u ^ u' => (-g => -g')))        "; ../build/quabs/quabs "examples/3_property.cir"; true
	@printf "Base       (E(s') -(i' ^ u' => -g'))                      "; ../build/quabs/quabs "examples/4_base.cir"; true
	@printf "Step       (E(s') -(-g0' ^ t' ^ u0' ^ u1' => -g1'))       "; ../build/quabs/quabs "examples/5_step.cir"; true

	@./build/bin/dimsim examples/model3.dimspec examples/witness3.dimtrace; result=$$?; \
		printf "Simulation "; if [ "$$result" = "0" ]; then echo "OKAY"; else echo "FAIL $$result"; fi; true
