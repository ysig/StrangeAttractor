# StrangeAttractor
Chaotic Modules for vcv rack

# Frac
Why not use fractal signals as vcos?
Is there an aesthetic characteristic in *fractalistic* signals?

This idea is explored with **Frac** a fractal vco that generates signals of two types:

- [takagi](https://en.wikipedia.org/wiki/Blancmange_curve`)
- [Weierstrass](https://en.wikipedia.org/wiki/Weierstrass_function)

# Attractor
Why not use chaotic attractors for modulating signals?

For that we developed almost all the attractors found [here](https://examples.pyviz.org/attractors/attractors.html), namely:

1. Clifford
2. De-Jong
3. Svensson
4. Bedhead
5. Dream
6. Hopalong-A
7. Hopalong-B
8. Gumowski-Mira


I'am new in audio-synthesis with vcv-rack so all patches are under development.
I will be happy to receive comments and concerns if any.

# Rabits
Rabits is vco based on the logistic growth function responsible for a lot of fundamental fractal obsevation.
Contains four parameters (r, Rate-of-Change (similar to frequency), volume and reset-button).
For this module I was inspired by [a veritasium video](https://www.youtube.com/watch?v=ovJcsL7vyrk) and [jonnyhyman git page](https://github.com/jonnyhyman/Chaos)
(Note points interpolated with lines and this gives a sort of triangular harmonic texture to the sound).
