#include <zoo/any.h>

#include <iostream>
#include <unordered_set>

std::unordered_set<void *> registrations;

struct Registers {
	double a, b;

	Registers() {
		std::cout << "Registers(" << this << ")\n";
		if(registrations.count(this)) {
			std::cout << "error C" << this << std::endl;
			throw 0;
		}
		registrations.insert(this);
	}

	Registers(const Registers &model) {
		std::cout << "Registers(" << this << ", " << &model << ")\n";
		if(registrations.count(this)) {
			std::cout << "error CC" << this << std::endl;
			throw 0;
		}
		registrations.insert(this);	
	}

	~Registers() {
		std::cout << "~Registers(" << this << ")\n";
		if(!registrations.count(this)) {
			std::cout << "error D" << this << std::endl;
		}
	}
};

int main(int argc, const char *argv[]) {
	zoo::Any bla{Registers{}};
}
