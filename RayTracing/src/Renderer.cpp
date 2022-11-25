#include "Renderer.h"
#include "Walnut/Random.h"

namespace Utils {

	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		//   alpha      blue       green       red
		// 1111 1111  1111 1111  1111 1111  1111 1111
		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height) // If image same size of window
			return; // No resize necessary

		m_FinalImage->Resize(width, height);
	}
	else
	{
		// Create image
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height]; // Create array of pixels
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	Ray ray;
	ray.Origin = camera.GetPosition();

	// We start with the y because of the CPU memory buffer layout:
	// As we go one uint32_t forward in memory, we are going horizontally across the image
	// If we flip these for loop, it would be weird because we're skipping a bunch of memory (full row forward)
	// --> slow down program
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
		{
			ray.Direction = camera.GetRayDirections()[x + y * m_FinalImage->GetWidth()];

			glm::vec4 color = TraceRay(scene, ray); // Called per pixel
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));

			// Multiply y by width to get the n-row of the buffer
			m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData); // Upload data to the GPU
}

glm::vec4 Renderer::TraceRay(const Scene& scene, const Ray& ray)
{
	// at^2 + bt + c = 0
	// (bx^2 + by^2)t^2 + (2(axbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray direction
	// r = radius
	// t = hit distance

	if (scene.Spheres.size() == 0)
		return glm::vec4(0, 0, 0, 1); // Black --> Background

	const Sphere* closestSphere = nullptr;
	float hitDistance = FLT_MAX;

	for (const Sphere& sphere : scene.Spheres)
	{
		glm::vec3 origin = ray.Origin - sphere.Position;

		//float a = std::pow(rayDirection.x, 2) + std::pow(rayDirection.y, 2) + std::pow(rayDirection.z, 2);
		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic formula discriminant:
		// b^2 - 4ac

		// (-b +- sqrt(discriminant)) / 2a

		float discriminant = b * b - 4.f * a * c;

		if (discriminant < 0.f)
			continue;

		//float t0 = (-b + glm::sqrt(discriminant)) / (2.f * a);
		float closestT = (-b - glm::sqrt(discriminant)) / (2.f * a); // This will be the smallest number

		// Find the closest hit sphere in case the ray hit multiple spheres
		if (closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = &sphere;
		}
	}

	if (!closestSphere)
		return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	glm::vec3 origin = ray.Origin - closestSphere->Position;

	// Calc hit positions
	glm::vec3 hitPoint = origin + ray.Direction * hitDistance;
	glm::vec3 normal = glm::normalize(hitPoint);

	glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));

	float d = glm::max(glm::dot(normal, -lightDir), 0.0f); // == cos(angle)

	glm::vec3 sphereColor = closestSphere->Albedo;
	sphereColor *= d;
	return glm::vec4(sphereColor, 1.0f);
}
