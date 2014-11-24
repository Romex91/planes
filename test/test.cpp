#include <geometry.h>
#include <database.h>
#include <configuration.h>
#include <database.h>
#include <modules.h>
#include <model.h>
#include <profile.h>


#include <omp.h>
#include <iostream>
#include <windows.h>
#include <ctime>
#include <ratio>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdlib>


#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>	

#define _USE_MATH_DEFINES
#include <math.h>

#include <sstream>

using namespace rplanes;

int HitzonesDemonstration()
{
	setlocale(LC_ALL, "rus");

	rplanes::playerdata::Profile Profile;
	auto db = rplanes::loadDatabase("planes.db");
	Profile.money = 10000000;
	Profile.buyPlane("me262", db);

	auto Plane = Profile.planes.back().buildPlane(Profile.pilot, db);
	Plane.initModules();

	sf::RenderWindow window(sf::VideoMode(300, 300), "SFML window");
	window.setFramerateLimit(25);

	sf::View camera;
	camera.reset(sf::FloatRect(0, 0, 150, 150));
	camera.setCenter(0, 0);
	window.setView(camera);


	float roll = -90;
	float mouseRoll = 0;
	sf::Text text;
	std::stringstream ss;

	sf::Vector2f mousePos;
	sf::VertexArray line(sf::LinesStrip, 2);
	line[0].position = sf::Vector2f(-10.f, -10.f);
	line[1].position = sf::Vector2f(10.f, 10.f);
	line[0].color = sf::Color::Red;
	line[1].color = sf::Color::Red;
	int sign = 1;
	int frameNum = 100;
	while (window.isOpen())
	{
		frameNum++;
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		mousePos = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(150, 150);
		sf::VertexArray animatedLine = line;
		animatedLine[0].position = sf::Transform().rotate(mouseRoll, sf::Vector2f(0.f, 0.f)).transformPoint(animatedLine[0].position) +
			mousePos;
		animatedLine[1].position = sf::Transform().rotate(mouseRoll, sf::Vector2f(0.f, 0.f)).transformPoint(animatedLine[1].position) +
			mousePos;

		window.clear(sf::Color::White);
		for (auto i = Plane.modules.begin(); i != Plane.modules.end(); i++)
		{
			(*i)->hitZone.spin(roll, roll);
		}
		if (roll >= 80.0)
		{
			sign = -1;
		}
		if (roll <= -80)
		{
			sign = 1;
		}
		roll += sign * 5;
		window.draw(animatedLine);
		sf::Vertex vert;
		for (auto i = Plane.modules.begin(); i != Plane.modules.end(); i++)
		{
			sf::ConvexShape sh;
			auto & Points = (*i)->hitZone.shape.points;
			sh.setPointCount(static_cast<unsigned int>(Points.size()));
			for (size_t i = 0; i < Points.size(); i++)
			{
				sh.setPoint(static_cast<unsigned int>(i), sf::Vector2f(Points[i].x * 10, Points[i].y * 10));
			}
			sh.setFillColor(sf::Color(255, 0, 0, 150));
			window.draw(sh);
		}
		window.draw(&vert, 1, sf::PrimitiveType::Points);
		window.display();
	}
	return 0;
}


int main1()
{
	omp_set_num_threads(2);
#pragma omp parallel sections
	{
#pragma omp section
		{
			while (true)
			{
#pragma omp parallel sections num_threads(2)
				{
#pragma omp section
					{
						std::cout << "a";
					}
#pragma omp section
					{
					Sleep(50);
					std::cout << "b";
				}
#pragma omp section
					{
						Sleep(100);
						std::cout << "c" << std::endl;
					}
				}
				Sleep(4000);
			}
		}
#pragma omp section
		{
		Sleep(1000);
		while (true)
		{
			std::cout << "2" << std::endl;
			Sleep(4000);
		}

	}
#pragma omp section
		{
			Sleep(2000);
			while (true)
			{
				std::cout << "3" << std::endl;
				Sleep(4000);
			}
		}
#pragma omp section
		{
			Sleep(3000);
			while (true)
			{
				std::cout << "4" << std::endl;
				Sleep(4000);
			}
		}
	}
};
int main2()
{
	omp_set_num_threads(2);
	omp_set_nested(true);
#pragma omp parallel sections
	{
#pragma omp section
		{
			while (true)
			{
#pragma omp parallel num_threads(5)
				{
#pragma omp for
					for (int n = 0; n < 10; n++)
					{
						std::cout << omp_get_thread_num() << std::endl;
					}
					std::cout << "---------------------------" << std::endl;
					Sleep(1000);
				}
			}
		}
#pragma omp section
		{
		while (true)
		{
			std::cout << "section 2" << std::endl;
			Sleep(3000);
		}
	}
	}
}

int main3()
{
	HitzonesDemonstration();
	rplanes::PointXY a(-1, -1);
	rplanes::PointXY b(1, 1);
	rplanes::PointXY c(0, -1);
	rplanes::PointXY d(0, -1);
	rplanes::PointXY e;

	if (rplanes::getLineSegmentsIntersection(a, b, c, d, e))
	{
		std::cout << "!" << std::endl;
	}
	system("pause");
}

// reflections
int main5()
{
	std::cout << -1 % 10 << std::endl;
 	sf::RenderWindow window(sf::VideoMode(300, 300), "SFML window");
	window.setFramerateLimit(30);

	sf::Font font;
	if (!font.loadFromFile("arial.ttf"))
		return EXIT_FAILURE;
	sf::Text text("", font, 20);
	text.setColor(sf::Color::Red);

	sf::VertexArray basicLine(sf::LinesStrip, 2);
	basicLine[0].position = sf::Vector2f(150, 150);
	basicLine[1].position = sf::Vector2f(300, 150);

	for (unsigned int i = 0; i < 2; i++)
	{
		basicLine[i].color = sf::Color::Red;
	}

	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		auto mousePos = sf::Vector2f(sf::Mouse::getPosition(window));


		sf::VertexArray
			ray(basicLine),
			mirror(basicLine),
			reflection(basicLine);

		mirror[0].position = sf::Vector2f(0, 150);

		for (unsigned int i = 0; i < 2; i++)
		{
			mirror[i].position = sf::Transform()
				.rotate(mousePos.y, sf::Vector2f(150.f, 150.f))
				.transformPoint(mirror[i].position);
			mirror[i].color = sf::Color::Black;
		}

		ray[1].position = sf::Transform().rotate(mousePos.x, sf::Vector2f(150.f, 150.f)).transformPoint(ray[1].position);


		std::stringstream ss;

		PointXY b(ray[0].position.x, ray[0].position.y);
		PointXY a(ray[1].position.x, ray[1].position.y);

		PointXY d(mirror[0].position.x, mirror[0].position.y);
		PointXY c(mirror[1].position.x, mirror[1].position.y);


		reflection = ray;
		float angle =  (angleFromPoints(a, b) - angleFromPoints(c, d));

		ss << sin(angle * M_PI / 180) << std::endl;
		ss << cos(angle * M_PI / 180);
		text.setString(ss.str());

		reflection[1].position = sf::Transform()
			.rotate(180 - 2 * angle, sf::Vector2f(150.f, 150.f))
			.transformPoint(reflection[1].position);

		reflection[0].color = sf::Color::Blue;
		reflection[1].color = sf::Color::Blue;

		text.setString(ss.str());

		window.clear(sf::Color::White);
		window.draw(ray);
		window.draw(mirror);
		window.draw(reflection);
		window.draw(text);
		window.display();
	}
}


// inner angle
int main()
{

	sf::RenderWindow window(sf::VideoMode(300, 300), "SFML window");
	window.setFramerateLimit(30);

	sf::Font font;
	if (!font.loadFromFile("../Resources/arial.ttf"))
		return EXIT_FAILURE;
	sf::Text text("", font, 20);
	text.setColor(sf::Color::Red);

	sf::VertexArray basicLine(sf::LinesStrip, 2);
	basicLine[0].position = sf::Vector2f(150, 150);
	basicLine[1].position = sf::Vector2f(300, 150);

	for (unsigned int i = 0; i < 2; i++)
	{
		basicLine[i].color = sf::Color::Red;
	}

	sf::CircleShape circle( 10 );
	circle.setFillColor(sf::Color::Green);
	circle.setPosition(150.f,150.f);

	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		auto mousePos = sf::Vector2f(sf::Mouse::getPosition(window));


		sf::VertexArray
			ray1(basicLine),
			ray2(basicLine);



		ray1[1].position = sf::Transform().rotate(mousePos.x, sf::Vector2f(150.f, 150.f)).transformPoint(ray1[1].position);
		ray2[1].position = sf::Transform().rotate(mousePos.y, sf::Vector2f(150.f, 150.f)).transformPoint(ray2[1].position);

		ray2[0].color = sf::Color::Black;
		ray2[1].color = sf::Color::Black;

		std::stringstream ss;

		PointXY a(ray1[0].position.x, ray1[0].position.y);
		PointXY b(ray1[1].position.x, ray1[1].position.y);

		PointXY c(ray2[0].position.x, ray2[0].position.y);
		PointXY d(ray2[1].position.x, ray2[1].position.y);

		float angle = (angleFromPoints(a, b) - angleFromPoints(c, d));

		//if ( angle < 0.f )
		//{
		//	angle += 360;
		//}

		ss << sin(angle * M_PI / 180) << std::endl;
		ss << cos(angle * M_PI / 180) << std::endl;
		ss << angle << std::endl;
		text.setString(ss.str());


		float direction = angleFromPoints(a, b) * M_PI / 180.f ;
		sf::Vector2f offset;
		offset.x = cos(direction) * 0.1f ;
		offset.y = sin(direction) * 0.1f;
		circle.move( offset );

		text.setString(ss.str());

		window.clear(sf::Color::White);
		window.draw(ray1);
		window.draw(ray2);
		window.draw(text);
		window.draw(circle);
		window.display();
	}
}

// ellips
int main4()
{

	sf::RenderWindow window(sf::VideoMode(300, 300), "SFML window");
	window.setFramerateLimit(30);

	sf::Font font;
	if (!font.loadFromFile("arial.ttf"))
		return EXIT_FAILURE;
	sf::Text text("", font, 20);

	text.setColor(sf::Color::Red);

	sf::CircleShape circle(2);
	circle.setFillColor(sf::Color::Green);

	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		auto mousePos = sf::Mouse::getPosition(window);

		float roll = mousePos.x % 180 - 90.f;
		float angle = mousePos.y % 360 ;

		std::stringstream ss;
		ss << "roll " << roll << std::endl;
		ss << "angle " << angle << std::endl;
		text.setString(ss.str());

		window.clear(sf::Color::White);
		
		std::vector< sf::Vector2f > points;

		for (int i = -5; i <= 5; i++)
		{
			points.push_back(sf::Vector2f( i * 5.f, 0.f ));
		}
		for ( auto & point : points )
		{

			//вычисляем угол склонения турели
			float beta = angle / 180 * M_PI;
			float theta = roll / 180 * M_PI;
			float alpha = atan( tan(beta) * cos(theta) ) / M_PI * 180;
			//вращаем турель
			point = sf::Transform().rotate(alpha, 0.f, 0.f).transformPoint(point);
			//наклоняем самолет
			sf::Vector2f xz(point.x, 0);
			xz = sf::Transform().rotate(roll, 0.f, 0.f).transformPoint(xz);
			point.x = xz.x;

			sf::Vector2f circlePosition(point);
			circle.setPosition(circlePosition.x + 150, circlePosition.y + 150);
			window.draw(circle);
		}
		window.draw(text);
		window.display();
	}
}

