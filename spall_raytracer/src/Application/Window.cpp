// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Application/Window.h>

LRESULT CALLBACK Window::procedure(
	HWND window,
	UINT message,
	WPARAM wordParameter,
	LPARAM longParameter)
{
	switch (message)
	{
		case WM_NCCREATE:
		{
			const CREATESTRUCTW* const create = reinterpret_cast<const CREATESTRUCTW*>(longParameter);
			SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
			break;
		}

		case WM_CLOSE:
		{
			Window* const self = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));

			if (self != nullptr)
			{
				self->m_Closed = true;
			}

			return 0;
		}

		default:
		{
			break;
		}
	}

	return DefWindowProcW(window, message, wordParameter, longParameter);
}

Window::~Window(
	void)
{
	if (m_Handle != nullptr)
	{
		DestroyWindow(m_Handle);
	}
}

spall::Status Window::open(
	std::uint32_t width,
	std::uint32_t height,
	const wchar_t* title)
{
	const HINSTANCE instance = GetModuleHandleW(nullptr);

	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = procedure;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"SpallRaytracer";
	windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

	RegisterClassW(&windowClass);

	constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

	RECT bounds = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	AdjustWindowRect(&bounds, windowStyle, FALSE);

	m_Handle = CreateWindowExW(
		0,
		L"SpallRaytracer",
		title,
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		bounds.right - bounds.left,
		bounds.bottom - bounds.top,
		nullptr,
		nullptr,
		instance,
		this);

	if (m_Handle == nullptr)
	{
		return spall::ERR_INVALID_WINDOW;
	}

	ShowWindow(m_Handle, SW_SHOW);

	return {};
}

void Window::pump(
	void)
{
	MSG message = {};

	while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

bool Window::closed(
	void) const
{
	return m_Closed;
}

HWND Window::handle(
	void) const
{
	return m_Handle;
}
