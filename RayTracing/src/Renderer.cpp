#include "Renderer.h"
#include "Walnut/Random.h"


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

void Renderer::Render()
{

	for (uint32_t i = 0; i < m_FinalImage->GetWidth() * m_FinalImage->GetHeight(); i++)
	{
		//m_ImageData[i] = 0xffff00ff; // ABGR - Magenta
		m_ImageData[i] = Walnut::Random::UInt();
		m_ImageData[i] |= 0xff000000; // Fill the "A" channel with 1
	}

	m_FinalImage->SetData(m_ImageData); // Upload data to the GPU
}