// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>

#include <windows.h>

#include <cstdint>

class Window
{
public:
	~Window(void);

	spall::Status open(
		std::uint32_t width,
		std::uint32_t height,
		const wchar_t* title);

	void pump(void);
	bool closed(void) const;

	HWND handle(void) const;

private:
	static LRESULT CALLBACK procedure(
		HWND window,
		UINT message,
		WPARAM wordParameter,
		LPARAM longParameter);

	HWND m_Handle = nullptr;
	bool m_Closed = false;
};
