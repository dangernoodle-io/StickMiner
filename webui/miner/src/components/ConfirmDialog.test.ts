import { describe, it, expect, beforeEach, vi } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/svelte'
import ConfirmDialog from './ConfirmDialog.svelte'

describe('ConfirmDialog', () => {
  beforeEach(() => {
    try { localStorage.clear() } catch { /* jsdom storage not always available */ }
    vi.clearAllMocks()
  })

  it('renders when open=true', () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Test Title',
        message: 'Test message'
      }
    })
    expect(screen.getByText('Test Title')).toBeInTheDocument()
    expect(screen.getByText('Test message')).toBeInTheDocument()
  })

  it('does not render when open=false', () => {
    render(ConfirmDialog, {
      props: {
        open: false,
        title: 'Hidden Title',
        message: 'Hidden message'
      }
    })
    expect(screen.queryByText('Hidden Title')).not.toBeInTheDocument()
  })

  it('writes to localStorage when skipKey is set and checkbox is checked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Are you sure?',
        message: 'Test',
        skipKey: 'test-skip'
      }
    })

    const checkbox = screen.getByRole('checkbox') as HTMLInputElement
    await fireEvent.click(checkbox)

    const buttons = screen.getAllByRole('button')
    const confirmBtn = buttons[buttons.length - 1]
    await fireEvent.click(confirmBtn)

    expect(localStorage.getItem('test-skip')).toBe('1')
  })

  it('does not write to localStorage if checkbox unchecked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Are you sure?',
        message: 'Test',
        skipKey: 'test-skip'
      }
    })

    const buttons = screen.getAllByRole('button')
    const confirmBtn = buttons[buttons.length - 1]
    await fireEvent.click(confirmBtn)

    expect(localStorage.getItem('test-skip')).toBeNull()
  })

  it('hides dialog when confirm button is clicked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Are you sure?',
        message: 'Test'
      }
    })

    expect(screen.getByText('Are you sure?')).toBeInTheDocument()

    const confirmBtn = screen.getByRole('button', { name: 'Confirm' })
    await fireEvent.click(confirmBtn)

    // confirm() sets open=false → title disappears from DOM
    expect(screen.queryByText('Are you sure?')).not.toBeInTheDocument()
  })

  it('hides dialog when cancel button is clicked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Are you sure?',
        message: 'Test'
      }
    })

    expect(screen.getByText('Are you sure?')).toBeInTheDocument()

    const cancelBtn = screen.getByRole('button', { name: 'Cancel' })
    await fireEvent.click(cancelBtn)

    // cancel() sets open=false → title disappears from DOM
    expect(screen.queryByText('Are you sure?')).not.toBeInTheDocument()
  })

  it('hides skipKey checkbox when not provided', () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Are you sure?',
        message: 'Test'
      }
    })

    expect(screen.queryByText("Don't show this again")).not.toBeInTheDocument()
  })
})
