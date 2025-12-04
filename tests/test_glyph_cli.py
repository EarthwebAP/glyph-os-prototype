#!/usr/bin/env python3
"""
Unit tests for create_glyph and query_glyph CLI tools
"""

import hashlib
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

# Add runtime/cli to path
sys.path.insert(0, str(Path(__file__).parent / ".." / "runtime" / "cli"))

import create_glyph
import query_glyph


class TestGlyphCLI(unittest.TestCase):
    """Test cases for glyph creation and querying"""

    def setUp(self):
        """Set up test fixtures"""
        # Create a temporary directory for testing
        self.test_dir = tempfile.mkdtemp()
        self.original_persistence_path = create_glyph.get_persistence_path

        # Override get_persistence_path for both modules
        def mock_persistence_path():
            return Path(self.test_dir)

        create_glyph.get_persistence_path = mock_persistence_path
        query_glyph.get_persistence_path = mock_persistence_path

    def tearDown(self):
        """Clean up test fixtures"""
        # Restore original function
        create_glyph.get_persistence_path = self.original_persistence_path
        query_glyph.get_persistence_path = self.original_persistence_path

        # Clean up temporary directory
        import shutil
        shutil.rmtree(self.test_dir)

    def test_create_glyph(self):
        """Test creating a glyph with SHA256 content addressing"""
        # Test data
        test_content = "Hello, Glyph World!"
        test_metadata = {"author": "test_user", "version": "1.0"}

        # Create glyph
        glyph_id, glyph_data = create_glyph.create_glyph(test_content, test_metadata)

        # Verify ID is SHA256 hash of content
        expected_id = hashlib.sha256(test_content.encode('utf-8')).hexdigest()
        self.assertEqual(glyph_id, expected_id)

        # Verify glyph structure
        self.assertEqual(glyph_data["id"], expected_id)
        self.assertEqual(glyph_data["content"], test_content)
        self.assertEqual(glyph_data["metadata"], test_metadata)

        # Save glyph and verify file exists
        file_path = create_glyph.save_glyph(glyph_id, glyph_data)
        self.assertTrue(os.path.exists(file_path))

        # Verify file content
        with open(file_path, 'r') as f:
            saved_data = json.load(f)
        self.assertEqual(saved_data, glyph_data)

    def test_query_glyph(self):
        """Test querying a glyph by ID"""
        # First, create a glyph
        test_content = "Query test glyph"
        test_metadata = {"type": "test"}

        glyph_id, glyph_data = create_glyph.create_glyph(test_content, test_metadata)
        create_glyph.save_glyph(glyph_id, glyph_data)

        # Now query it
        queried_data = query_glyph.query_glyph(glyph_id)

        # Verify queried data matches created data
        self.assertIsNotNone(queried_data)
        self.assertEqual(queried_data, glyph_data)
        self.assertEqual(queried_data["id"], glyph_id)
        self.assertEqual(queried_data["content"], test_content)
        self.assertEqual(queried_data["metadata"], test_metadata)

    def test_query_nonexistent_glyph(self):
        """Test querying a glyph that doesn't exist"""
        fake_id = "nonexistent123456789abcdef"
        queried_data = query_glyph.query_glyph(fake_id)

        # Should return None for non-existent glyph
        self.assertIsNone(queried_data)

    def test_content_addressing_consistency(self):
        """Test that same content produces same ID"""
        content1 = "Identical content"
        content2 = "Identical content"

        id1, _ = create_glyph.create_glyph(content1)
        id2, _ = create_glyph.create_glyph(content2)

        # Same content should produce same ID
        self.assertEqual(id1, id2)

    def test_different_content_different_ids(self):
        """Test that different content produces different IDs"""
        content1 = "First glyph"
        content2 = "Second glyph"

        id1, _ = create_glyph.create_glyph(content1)
        id2, _ = create_glyph.create_glyph(content2)

        # Different content should produce different IDs
        self.assertNotEqual(id1, id2)


if __name__ == "__main__":
    unittest.main()
